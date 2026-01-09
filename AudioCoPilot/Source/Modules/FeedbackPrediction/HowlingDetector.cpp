#include "HowlingDetector.h"
#include <algorithm>
#include <cmath>

namespace AudioCoPilot {

HowlingDetector::HowlingDetector() {
    _magnitudeHistory.resize(HowlingDetectorConfig::HISTORY_FRAMES);
}

void HowlingDetector::prepare(double sampleRate, int fftSize) {
    _sampleRate = sampleRate;
    _fftSize = fftSize;
    
    const int numBins = _fftSize / 2 + 1;
    
    // Inicializa histórico
    for (auto& frame : _magnitudeHistory) {
        frame.resize(numBins, -100.0f);
    }
    
    // Inicializa persistência
    _binPersistence.resize(numBins, 0);
    
    // Inicializa candidatos
    for (auto& c : _candidates) {
        c = FeedbackCandidate{};
        c.status = FeedbackCandidate::Status::Safe;
    }
    
    // Pré-aloca buffers temporários (Otimização)
    _refFrameDB.resize(numBins, -100.0f);
    _peakBins.reserve(numBins / 4); // Reserva espaço para ~25% dos bins como picos
    _slopeValues.reserve(HowlingDetectorConfig::HISTORY_FRAMES);
    
    DBG("HowlingDetector::prepare() - sampleRate: " << sampleRate << ", fftSize: " << fftSize);
}

void HowlingDetector::reset() {
    for (auto& frame : _magnitudeHistory) {
        std::fill(frame.begin(), frame.end(), -100.0f);
    }
    std::fill(_binPersistence.begin(), _binPersistence.end(), 0);
    std::fill(_refFrameDB.begin(), _refFrameDB.end(), -100.0f);
    _peakBins.clear();
    _slopeValues.clear();
    _historyIndex = 0;
    
    // Reset adaptive thresholding
    _rmsMeasurement = HowlingDetectorConfig::RMS_MIN;
    _rmsReference = HowlingDetectorConfig::RMS_MIN;
    _thresholdPNPR = HowlingDetectorConfig::THRESHOLD_PNPR_BASE;
    _thresholdPHPR = HowlingDetectorConfig::THRESHOLD_PHPR_BASE;
    
    for (auto& c : _candidates) {
        c = FeedbackCandidate{};
        c.status = FeedbackCandidate::Status::Safe;
    }
}

void HowlingDetector::processFrame(const float* magnitudeSpectrum, int numBins) {
    // Chama a versão com referência, passando nullptr
    processFrameWithReference(magnitudeSpectrum, nullptr, numBins);
}

void HowlingDetector::processFrameWithReference(const float* measurementSpectrum, 
                                              const float* referenceSpectrum, 
                                              int numBins) {
    // Inicia monitoramento de performance
    _performanceMonitor.startProcessing();
    
    // 0. Atualiza RMS e thresholds adaptativos
    updateRMS(measurementSpectrum, referenceSpectrum, numBins);
    
    // 1. Armazena frame atual no histórico (em dB)
    auto& currentFrame = _magnitudeHistory[_historyIndex];
    
    // Limpa buffers temporários
    _peakBins.clear();
    
    for (int i = 0; i < numBins && i < static_cast<int>(currentFrame.size()); ++i) {
        // Converte Measurement para dB
        float mag = measurementSpectrum[i];
        currentFrame[i] = (mag > 1e-10f) ? 20.0f * std::log10(mag) : -100.0f;
        
        // Converte Reference para dB (se existir) no buffer pré-alocado
        if (referenceSpectrum) {
            float refMag = referenceSpectrum[i];
            _refFrameDB[i] = (refMag > 1e-10f) ? 20.0f * std::log10(refMag) : -100.0f;
        } else {
            _refFrameDB[i] = -100.0f;
        }
    }
    
    // 2. Encontra picos no espectro de medição
    findPeaks(currentFrame.data(), numBins);
    
    // 3. Atualiza persistência
    for (auto& p : _binPersistence) {
        if (p > 0) p--;
    }
    for (int bin : _peakBins) {
        if (bin >= 0 && bin < static_cast<int>(_binPersistence.size())) {
            _binPersistence[bin] = std::min(_binPersistence[bin] + 2, 
                                         HowlingDetectorConfig::HISTORY_FRAMES * 2);
        }
    }
    
    // 4. Se tivermos Referência, filtramos picos que também estão presentes nela
    // Se (Meas - Ref) < Threshold, o pico provavelmente faz parte do sinal original (música)
    if (referenceSpectrum) {
        // Remove picos que não excedem a referência significativamente
        _peakBins.erase(std::remove_if(_peakBins.begin(), _peakBins.end(), [&](int bin) {
            float diff = currentFrame[bin] - _refFrameDB[bin];
            
            // CRITÉRIO DE FILTRAGEM:
            // Se a diferença for negativa (Ref > Meas), certamente é seguro.
            // Se a diferença for pequena (< 3dB), provavelmente é seguro.
            // Só mantemos se Meas for pelo menos 3dB maior que Ref.
            return diff < 3.0f; 
        }), _peakBins.end());
    }
    
    // 5. Ordena por magnitude (ou por Loop Gain se tiver referência)
    std::sort(_peakBins.begin(), _peakBins.end(), [&](int a, int b) {
        if (referenceSpectrum) {
            float gainA = currentFrame[a] - _refFrameDB[a];
            float gainB = currentFrame[b] - _refFrameDB[b];
            return gainA > gainB;
        } else {
            return currentFrame[a] > currentFrame[b];
        }
    });
    
    // Limita ao número máximo de candidatos
    if (_peakBins.size() > static_cast<size_t>(HowlingDetectorConfig::MAX_PEAKS)) {
        _peakBins.resize(HowlingDetectorConfig::MAX_PEAKS);
    }
    
    // 6. Calcula features e preenche candidatos
    for (size_t i = 0; i < _candidates.size(); ++i) {
        if (i < _peakBins.size()) {
            int bin = _peakBins[i];
            if (bin >= 0 && bin < numBins && bin < static_cast<int>(currentFrame.size())) {
                auto& c = _candidates[i];
                
                c.frequency = binToFrequency(bin);
                c.magnitude = currentFrame[bin];
                
                if (referenceSpectrum) {
                    c.refMagnitude = _refFrameDB[bin];
                    c.loopGain = c.magnitude - c.refMagnitude;
                } else {
                    c.refMagnitude = -100.0f;
                    c.loopGain = 0.0f;
                }
                
                c.pnpr = calculatePNPR(currentFrame.data(), numBins, bin);
                c.phpr = calculatePHPR(currentFrame.data(), numBins, bin);
                c.persistence = (bin < static_cast<int>(_binPersistence.size())) 
                    ? _binPersistence[bin] : 0;
                c.slope = calculateSlope(bin);
                
                // Calcula score de risco
                c.riskScore = calculateRiskScore(c);
                
                // --- LÓGICA DE COMPARAÇÃO (REF vs MEAS) ---
                if (referenceSpectrum) {
                    // Se loop gain > 6dB, risco vai pra 1.0 rapidamente
                    // Isso significa que o mic está captando +6dB do que saiu da mesa
                    if (c.loopGain > 6.0f) {
                        // Mapeia gain de 6dB a 12dB para score 0.9 a 1.0
                        float extraRisk = std::clamp((c.loopGain - 6.0f) / 6.0f, 0.0f, 1.0f);
                        c.riskScore = std::max(c.riskScore, 0.9f + (extraRisk * 0.1f));
                    }
                    
                    // Se loop gain é baixo (< 3dB), reduz drasticamente o risco
                    // pois o pico provavelmente veio da fonte original
                    if (c.loopGain < 3.0f) {
                        c.riskScore *= 0.1f;
                    }
                }
                
                c.status = determineStatus(c.riskScore);
                
                if (isLearnedFrequency(c.frequency)) {
                    c.riskScore *= 0.5f;
                    c.status = determineStatus(c.riskScore);
                }
                
                // Logging para eventos importantes
                if (c.status == FeedbackCandidate::Status::Feedback) {
                    FB_LOG_CRITICAL("HowlingDetector", "FEEDBACK DETECTED", 
                                   c.frequency, c.riskScore, c.magnitude, c.loopGain);
                } else if (c.status == FeedbackCandidate::Status::Danger) {
                    FB_LOG_WARNING("HowlingDetector", "Dangerous frequency detected",
                                  c.frequency, c.riskScore, c.magnitude, c.loopGain);
                } else if (c.status == FeedbackCandidate::Status::Warning) {
                    FB_LOG_INFO("HowlingDetector", "Warning: potential feedback",
                               c.frequency, c.riskScore, c.magnitude, c.loopGain);
                }
            }
        } else {
            // Sem candidato neste slot
            _candidates[i] = FeedbackCandidate{};
            _candidates[i].status = FeedbackCandidate::Status::Safe;
        }
    }
    
    // Avança índice do histórico
    _historyIndex = (_historyIndex + 1) % HowlingDetectorConfig::HISTORY_FRAMES;
    
    // Registra eventos de performance
    for (const auto& c : _candidates) {
        if (c.status == FeedbackCandidate::Status::Feedback) {
            _performanceMonitor.recordFeedbackEvent(c.riskScore, c.loopGain);
        } else if (c.status == FeedbackCandidate::Status::Danger || 
                   c.status == FeedbackCandidate::Status::Warning) {
            _performanceMonitor.recordWarningEvent(c.riskScore);
        }
    }
    
    // Finaliza monitoramento de performance
    _performanceMonitor.endProcessing();
}

void HowlingDetector::findPeaks(const float* spectrum, int numBins) {
    _peakBins.clear();
    
    // Ignora DC e frequências muito altas
    const int minBin = frequencyToBin(80.0f);   
    const int maxBin = frequencyToBin(12000.0f); 
    
    for (int i = std::max(2, minBin); i < std::min(numBins - 2, maxBin); ++i) {
        if (spectrum[i] > spectrum[i-1] && 
            spectrum[i] > spectrum[i+1] &&
            spectrum[i] > spectrum[i-2] &&
            spectrum[i] > spectrum[i+2]) {
            
            // Threshold mais permissivo (-60dB) pois a referência vai filtrar
            if (spectrum[i] > -60.0f) { 
                _peakBins.push_back(i);
            }
        }
    }
}

float HowlingDetector::calculatePNPR(const float* spectrum, int numBins, int peakBin) {
    const int m = HowlingDetectorConfig::NEIGHBOR_BINS;
    float peakPower = spectrum[peakBin];
    
    float neighborSum = 0.0f;
    int neighborCount = 0;
    
    for (int offset = -m; offset <= m; ++offset) {
        if (offset == 0) continue;
        int bin = peakBin + offset;
        if (bin >= 0 && bin < numBins) {
            neighborSum += spectrum[bin];
            neighborCount++;
        }
    }
    
    float neighborAvg = (neighborCount > 0) ? neighborSum / neighborCount : -100.0f;
    return peakPower - neighborAvg;
}

float HowlingDetector::calculatePHPR(const float* spectrum, int numBins, int peakBin) {
    float peakPower = spectrum[peakBin];
    int harmonicBin = peakBin * 2;
    
    if (harmonicBin >= numBins) return 40.0f;
    
    float harmonicPower = -100.0f;
    for (int offset = -3; offset <= 3; ++offset) {
        int bin = harmonicBin + offset;
        if (bin >= 0 && bin < numBins) {
            harmonicPower = std::max(harmonicPower, spectrum[bin]);
        }
    }
    
    return peakPower - harmonicPower;
}

float HowlingDetector::calculateSlope(int bin) {
    if (_magnitudeHistory.empty()) return 0.0f;
    
    std::vector<float> values;
    values.reserve(HowlingDetectorConfig::HISTORY_FRAMES);
    
    for (int i = 0; i < HowlingDetectorConfig::HISTORY_FRAMES; ++i) {
        int idx = (_historyIndex - 1 - i + HowlingDetectorConfig::HISTORY_FRAMES) 
                  % HowlingDetectorConfig::HISTORY_FRAMES;
        if (bin < static_cast<int>(_magnitudeHistory[idx].size())) {
            values.push_back(_magnitudeHistory[idx][bin]);
        }
    }
    
    if (values.size() < 4) return 0.0f;
    
    // Regressão linear simples
    float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    int n = static_cast<int>(values.size());
    
    for (int i = 0; i < n; ++i) {
        float x = static_cast<float>(i);
        float y = values[i];
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }
    
    float denom = n * sumX2 - sumX * sumX;
    if (std::abs(denom) < 1e-10f) return 0.0f;
    
    return (n * sumXY - sumX * sumY) / denom;
}

float HowlingDetector::calculateRiskScore(const FeedbackCandidate& c) {
    float score = 0.0f;
    
    // PNPR (0-0.3) - usa threshold adaptativo
    float pnprNorm = std::clamp(c.pnpr / _thresholdPNPR, 0.0f, 1.5f);
    score += pnprNorm * 0.25f;
    
    // PHPR (0-0.3) - usa threshold adaptativo
    float phprNorm = std::clamp(c.phpr / _thresholdPHPR, 0.0f, 1.5f);
    score += phprNorm * 0.25f;
    
    // Persistence (0-0.2)
    float persistNorm = std::clamp(static_cast<float>(c.persistence) / 
                                    static_cast<float>(HowlingDetectorConfig::MIN_PERSISTENCE), 
                                    0.0f, 1.5f);
    score += persistNorm * 0.2f;
    
    // Slope (0-0.2)
    if (c.slope > HowlingDetectorConfig::MIN_SLOPE) {
        float slopeNorm = std::clamp(c.slope / 3.0f, 0.0f, 1.0f);
        score += slopeNorm * 0.2f;
    }
    
    // Audibility weighting
    float audibility = getAudibilityWeight(c.frequency);
    score *= (0.7f + 0.3f * audibility);
    
    return std::clamp(score, 0.0f, 1.0f);
}

FeedbackCandidate::Status HowlingDetector::determineStatus(float riskScore) {
    if (riskScore >= 1.0f) {
        return FeedbackCandidate::Status::Feedback;
    } else if (riskScore >= HowlingDetectorConfig::DANGER_THRESHOLD) {
        return FeedbackCandidate::Status::Danger;
    } else if (riskScore >= HowlingDetectorConfig::WARNING_THRESHOLD) {
        return FeedbackCandidate::Status::Warning;
    }
    return FeedbackCandidate::Status::Safe;
}

float HowlingDetector::getAudibilityWeight(float frequency) const {
    if (frequency < 100.0f) return 0.2f;
    if (frequency < 500.0f) return 0.5f;
    if (frequency < 1000.0f) return 0.7f;
    if (frequency < 2000.0f) return 0.9f;
    if (frequency < 5000.0f) return 1.0f;
    if (frequency < 8000.0f) return 0.8f;
    return 0.5f;
}

float HowlingDetector::binToFrequency(int bin) const {
    return static_cast<float>(bin) * static_cast<float>(_sampleRate) / static_cast<float>(_fftSize);
}

int HowlingDetector::frequencyToBin(float freq) const {
    return static_cast<int>(freq * static_cast<float>(_fftSize) / static_cast<float>(_sampleRate));
}

const std::array<FeedbackCandidate, HowlingDetectorConfig::MAX_PEAKS>& 
HowlingDetector::getCandidates() const {
    return _candidates;
}

FeedbackCandidate::Status HowlingDetector::getWorstStatus() const {
    auto worst = FeedbackCandidate::Status::Safe;
    for (const auto& c : _candidates) {
        if (c.status > worst) worst = c.status;
    }
    return worst;
}

float HowlingDetector::getMostDangerousFrequency() const {
    float maxRisk = 0.0f;
    float freq = 0.0f;
    for (const auto& c : _candidates) {
        if (c.riskScore > maxRisk) {
            maxRisk = c.riskScore;
            freq = c.frequency;
        }
    }
    return freq;
}

void HowlingDetector::setLearnedFrequencies(const std::vector<float>& frequencies) {
    _learnedFrequencies = frequencies;
}

bool HowlingDetector::isLearnedFrequency(float freq, float tolerance) const {
    for (float learned : _learnedFrequencies) {
        if (std::abs(freq - learned) < tolerance) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// Adaptive Thresholding Implementation
// ============================================================================

float HowlingDetector::calculateRMS(const float* spectrum, int numBins) const {
    if (numBins <= 0) return HowlingDetectorConfig::RMS_MIN;
    
    float sumSquares = 0.0f;
    for (int i = 0; i < numBins; ++i) {
        sumSquares += spectrum[i] * spectrum[i];
    }
    
    float rms = std::sqrt(sumSquares / static_cast<float>(numBins));
    return std::max(rms, HowlingDetectorConfig::RMS_MIN);
}

float HowlingDetector::getAdaptiveThresholdScale(float rms) const {
    // Normaliza RMS em relação ao nível de referência
    float normalizedRMS = rms / HowlingDetectorConfig::RMS_REFERENCE;
    
    // Aplica função logarítmica suave para scaling
    // Para RMS baixo: scale ~ 0.5, para RMS alto: scale ~ 2.0
    float logScale = std::log10(std::max(normalizedRMS, 0.01f));
    
    // Mapeia logScale para o range desejado
    float scale = HowlingDetectorConfig::THRESHOLD_SCALE_LOW + 
                  (HowlingDetectorConfig::THRESHOLD_SCALE_HIGH - HowlingDetectorConfig::THRESHOLD_SCALE_LOW) *
                  (logScale + 2.0f) / 4.0f; // +2 para shift, /4 para scaling
    
    return std::clamp(scale, HowlingDetectorConfig::THRESHOLD_SCALE_LOW, 
                      HowlingDetectorConfig::THRESHOLD_SCALE_HIGH);
}

void HowlingDetector::updateRMS(const float* measurementSpectrum, const float* referenceSpectrum, int numBins) {
    // Calcula RMS atual
    float currentRMSMeas = calculateRMS(measurementSpectrum, numBins);
    
    // Atualiza RMS com suavização exponencial
    _rmsMeasurement = HowlingDetectorConfig::RMS_ALPHA * _rmsMeasurement + 
                      (1.0f - HowlingDetectorConfig::RMS_ALPHA) * currentRMSMeas;
    
    // Atualiza RMS da referência se disponível
    if (referenceSpectrum) {
        float currentRMSRef = calculateRMS(referenceSpectrum, numBins);
        _rmsReference = HowlingDetectorConfig::RMS_ALPHA * _rmsReference + 
                        (1.0f - HowlingDetectorConfig::RMS_ALPHA) * currentRMSRef;
    } else {
        _rmsReference = HowlingDetectorConfig::RMS_MIN;
    }
    
    // Atualiza thresholds adaptativos
    updateAdaptiveThresholds();
}

void HowlingDetector::updateAdaptiveThresholds() {
    // Usa o RMS do measurement para scaling (ou a média se tiver referência)
    float effectiveRMS = _rmsMeasurement;
    if (_rmsReference > HowlingDetectorConfig::RMS_MIN) {
        // Se temos referência, usamos a diferença RMS como indicador de ganho de loop
        effectiveRMS = std::max(_rmsMeasurement - _rmsReference, HowlingDetectorConfig::RMS_MIN);
    }
    
    float scale = getAdaptiveThresholdScale(effectiveRMS);
    
    // Aplica scaling aos thresholds base
    _thresholdPNPR = HowlingDetectorConfig::THRESHOLD_PNPR_BASE * scale;
    _thresholdPHPR = HowlingDetectorConfig::THRESHOLD_PHPR_BASE * scale;
    
    // Debug logging
    std::string debugMsg = "Adaptive Thresholds - RMS: " + std::to_string(effectiveRMS) +
                          ", Scale: " + std::to_string(scale) +
                          ", PNPR: " + std::to_string(_thresholdPNPR) + " dB" +
                          ", PHPR: " + std::to_string(_thresholdPHPR) + " dB";
    FB_LOG_DEBUG("HowlingDetector", debugMsg);
}

} // namespace AudioCoPilot
