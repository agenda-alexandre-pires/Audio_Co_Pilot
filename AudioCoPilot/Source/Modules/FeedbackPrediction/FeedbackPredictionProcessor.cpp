#include "FeedbackPredictionProcessor.h"
#include "../../Core/DebugLogger.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace AudioCoPilot {

FeedbackPredictionProcessor::FeedbackPredictionProcessor() {
    DEBUG_LOG("FeedbackPredictionProcessor constructor called");
    
    // Cria FFT setup
    _fftSetup = vDSP_create_fftsetup(_fftOrder, FFT_RADIX2);
    if (!_fftSetup) {
        DEBUG_ERROR("Failed to create FFT setup!");
    } else {
        DEBUG_LOG("FFT setup created successfully - order: " + juce::String(_fftOrder) + 
                 ", size: " + juce::String(_fftSize));
    }
    
    // Aloca buffers Measurement
    _inputBufferMeas.resize(_fftSize, 0.0f);
    _windowedBufferMeas.resize(_fftSize, 0.0f);
    _fftRealMeas.resize(_fftSize / 2, 0.0f);
    _fftImagMeas.resize(_fftSize / 2, 0.0f);
    _magnitudeSpectrumMeas.resize(_fftSize / 2 + 1, 0.0f);
    
    _splitComplexMeas.realp = _fftRealMeas.data();
    _splitComplexMeas.imagp = _fftImagMeas.data();
    
    // Aloca buffers Reference
    _inputBufferRef.resize(_fftSize, 0.0f);
    _windowedBufferRef.resize(_fftSize, 0.0f);
    _fftRealRef.resize(_fftSize / 2, 0.0f);
    _fftImagRef.resize(_fftSize / 2, 0.0f);
    _magnitudeSpectrumRef.resize(_fftSize / 2 + 1, 0.0f);
    
    _splitComplexRef.realp = _fftRealRef.data();
    _splitComplexRef.imagp = _fftImagRef.data();
    
    // Aloca buffer temporário para FFT (correção de casting)
    _fftTempBuffer.resize(_fftSize / 2);
    
    // Aloca buffer temporário para sqrt
    _sqrtTempBuffer.resize(_fftSize / 2);
    
    createHannWindow();
    
    DEBUG_LOG("FeedbackPredictionProcessor initialized - buffers allocated");
    DEBUG_LOG("  FFT Size: " + juce::String(_fftSize));
    DEBUG_LOG("  Hop Size: " + juce::String(_hopSize));
    DEBUG_LOG("  Input Buffer Meas size: " + juce::String(_inputBufferMeas.size()));
    DEBUG_LOG("  Input Buffer Ref size: " + juce::String(_inputBufferRef.size()));
}

FeedbackPredictionProcessor::~FeedbackPredictionProcessor() {
    if (_fftSetup) {
        vDSP_destroy_fftsetup(_fftSetup);
    }
}

void FeedbackPredictionProcessor::prepare(double sampleRate, int samplesPerBlock) {
    _sampleRate = sampleRate;
    _detector.prepare(sampleRate, _fftSize);
    DBG("FeedbackPredictionProcessor::prepare() - sampleRate: " << sampleRate << ", fftSize: " << _fftSize);
    reset();
}

void FeedbackPredictionProcessor::reset() {
    std::fill(_inputBufferMeas.begin(), _inputBufferMeas.end(), 0.0f);
    std::fill(_inputBufferRef.begin(), _inputBufferRef.end(), 0.0f);
    _inputWritePos = 0;
    _detector.reset();
    _learnFrameCount = 0;
}

void FeedbackPredictionProcessor::createHannWindow() {
    _hannWindow.resize(_fftSize);
    vDSP_hann_window(_hannWindow.data(), _fftSize, vDSP_HANN_NORM);
}

// Método antigo (compatibilidade)
void FeedbackPredictionProcessor::process(const float* inputBuffer, int numSamples) {
    process(inputBuffer, nullptr, numSamples);
}

// Novo método (Reference vs Measurement)
void FeedbackPredictionProcessor::process(const float* measurementBuffer, const float* referenceBuffer, int numSamples) {
    if (!_enabled.load()) {
        DEBUG_WARNING("FeedbackPredictionProcessor::process called but disabled");
        return;
    }
    
    DEBUG_PROFILE_START("FeedbackPredictionProcess");
    
    // DEBUG: Verificar parâmetros
    if (!measurementBuffer) {
        DEBUG_ERROR("FeedbackPredictionProcessor::process called with null measurementBuffer!");
        DEBUG_PROFILE_END("FeedbackPredictionProcess");
        return;
    }
    
    if (numSamples <= 0) {
        DEBUG_WARNING("FeedbackPredictionProcessor::process called with numSamples <= 0");
        DEBUG_PROFILE_END("FeedbackPredictionProcess");
        return;
    }
    
    // Processa em batch para melhor performance
    int samplesProcessed = 0;
    while (samplesProcessed < numSamples) {
        int samplesToCopy = std::min(_hopSize - _inputWritePos, numSamples - samplesProcessed);
        
        // DEBUG: Verificar limites
        if (_inputWritePos + samplesToCopy > static_cast<int>(_inputBufferMeas.size())) {
            DEBUG_ERROR("BUFFER OVERFLOW in FeedbackPredictionProcessor::process!");
            DEBUG_ERROR("  _inputWritePos: " + juce::String(_inputWritePos));
            DEBUG_ERROR("  samplesToCopy: " + juce::String(samplesToCopy));
            DEBUG_ERROR("  buffer size: " + juce::String(_inputBufferMeas.size()));
            DEBUG_PROFILE_END("FeedbackPredictionProcess");
            return;
        }
        
        // Copia samples em batch
        if (referenceBuffer) {
            std::memcpy(_inputBufferMeas.data() + _inputWritePos, 
                       measurementBuffer + samplesProcessed, 
                       samplesToCopy * sizeof(float));
            std::memcpy(_inputBufferRef.data() + _inputWritePos, 
                       referenceBuffer + samplesProcessed, 
                       samplesToCopy * sizeof(float));
        } else {
            std::memcpy(_inputBufferMeas.data() + _inputWritePos, 
                       measurementBuffer + samplesProcessed, 
                       samplesToCopy * sizeof(float));
            std::fill(_inputBufferRef.data() + _inputWritePos, 
                     _inputBufferRef.data() + _inputWritePos + samplesToCopy, 
                     0.0f);
        }
        
        _inputWritePos += samplesToCopy;
        samplesProcessed += samplesToCopy;
        
        // Quando temos um hop completo, processa FFT
        if (_inputWritePos >= _hopSize) {
            DEBUG_PROFILE_START("processFFT");
            processFFT(referenceBuffer != nullptr);
            DEBUG_PROFILE_END("processFFT");
            
            // Shift buffer Measurement
            std::memmove(_inputBufferMeas.data(), 
                        _inputBufferMeas.data() + _hopSize, 
                        (_fftSize - _hopSize) * sizeof(float));
            
            // Shift buffer Reference
            std::memmove(_inputBufferRef.data(), 
                        _inputBufferRef.data() + _hopSize, 
                        (_fftSize - _hopSize) * sizeof(float));
            
            _inputWritePos = _fftSize - _hopSize;
        }
    }
    
    DEBUG_PROFILE_END("FeedbackPredictionProcess");
}

void FeedbackPredictionProcessor::performSingleChannelFFT(const std::vector<float>& windowed, 
                                                        DSPSplitComplex& splitComplex,
                                                        std::vector<float>& magnitude) {
    // CORREÇÃO: Uso seguro de vDSP_ctoz sem casting perigoso
    // 1. Preenche buffer temporário com dados reais (parte imaginária = 0)
    for (size_t i = 0; i < _fftTempBuffer.size(); ++i) {
        _fftTempBuffer[i].real = windowed[i * 2];
        _fftTempBuffer[i].imag = 0.0f;
    }
    
    // 2. Converte para split complex format
    vDSP_ctoz(_fftTempBuffer.data(), 2, &splitComplex, 1, _fftSize / 2);
    
    // 3. Perform Real Forward FFT
    vDSP_fft_zrip(_fftSetup, &splitComplex, 1, _fftOrder, FFT_FORWARD);
    
    // 4. Calculate Magnitudes
    // |X|² = real² + imag²
    // Output of zrip is packed: DC is at real[0], Nyquist at imag[0]
    
    // Calculate squared magnitude (apenas para N/2 bins, Nyquist será tratado separadamente)
    int numBinsToProcess = _fftSize / 2;
    vDSP_zvmags(&splitComplex, 1, magnitude.data(), 1, numBinsToProcess);
    
    // 5. Normalize (1/N)
    float scale = 1.0f / static_cast<float>(_fftSize);
    vDSP_vsmul(magnitude.data(), 1, &scale, magnitude.data(), 1, numBinsToProcess);
    
    // 6. Sqrt to get linear magnitude (NÃO usar in-place com vvsqrtf!)
    vvsqrtf(_sqrtTempBuffer.data(), magnitude.data(), &numBinsToProcess);
    
    // Copia de volta para magnitude (apenas os primeiros N/2 bins)
    std::memcpy(magnitude.data(), _sqrtTempBuffer.data(), numBinsToProcess * sizeof(float));
    
    // 7. Trata o bin de Nyquist (está em imag[0] do resultado packed)
    // Para FFT real, o bin de Nyquist está apenas na parte real
    float nyquistMagnitude = std::sqrt(splitComplex.imagp[0] * splitComplex.imagp[0] * scale);
    magnitude[numBinsToProcess] = nyquistMagnitude; // Último bin (Nyquist)
    
    // Nota: magnitude[0] contém DC, magnitude[_fftSize/2] contém Nyquist
    // Para análise de feedback, usamos todos os bins
}

void FeedbackPredictionProcessor::processFFT(bool hasReference) {
    DEBUG_PROFILE_START("processFFT");
    
    static int fftCount = 0;
    fftCount++;
    
    // DEBUG
    if (fftCount % 50 == 0) {
        DEBUG_LOG("FFT processing #" + juce::String(fftCount) + 
                 " - hasReference: " + juce::String(hasReference ? "YES" : "NO"));
    }
    
    // --- 1. Process Measurement ---
    // Apply Window
    DEBUG_PROFILE_START("WindowMeasurement");
    vDSP_vmul(_inputBufferMeas.data(), 1, _hannWindow.data(), 1, _windowedBufferMeas.data(), 1, _fftSize);
    DEBUG_PROFILE_END("WindowMeasurement");
    
    DEBUG_PROFILE_START("FFTMeasurement");
    performSingleChannelFFT(_windowedBufferMeas, _splitComplexMeas, _magnitudeSpectrumMeas);
    DEBUG_PROFILE_END("FFTMeasurement");
    
    // --- 2. Process Reference (if available) ---
    if (hasReference) {
        DEBUG_PROFILE_START("WindowReference");
        vDSP_vmul(_inputBufferRef.data(), 1, _hannWindow.data(), 1, _windowedBufferRef.data(), 1, _fftSize);
        DEBUG_PROFILE_END("WindowReference");
        
        DEBUG_PROFILE_START("FFTReference");
        performSingleChannelFFT(_windowedBufferRef, _splitComplexRef, _magnitudeSpectrumRef);
        DEBUG_PROFILE_END("FFTReference");
        
        // Pass BOTH to detector
        DEBUG_PROFILE_START("DetectorWithReference");
        _detector.processFrameWithReference(_magnitudeSpectrumMeas.data(), 
                                          _magnitudeSpectrumRef.data(), 
                                          _fftSize / 2 + 1);
        DEBUG_PROFILE_END("DetectorWithReference");
    } else {
        // Pass only Measurement
        DEBUG_PROFILE_START("Detector");
        _detector.processFrame(_magnitudeSpectrumMeas.data(), _fftSize / 2 + 1);
        DEBUG_PROFILE_END("Detector");
    }
    
    // Learn Stage
    if (_learning.load()) {
        DEBUG_PROFILE_START("LearnFrame");
        processLearnFrame();
        DEBUG_PROFILE_END("LearnFrame");
    }
    
    DEBUG_PROFILE_END("processFFT");
}

void FeedbackPredictionProcessor::startLearning() {
    _learnedFrequencies.clear();
    _frequencyHitCount.resize(_fftSize / 2 + 1, 0);
    std::fill(_frequencyHitCount.begin(), _frequencyHitCount.end(), 0);
    _learnFrameCount = 0;
    _learning.store(true);
}

void FeedbackPredictionProcessor::stopLearning() {
    _learning.store(false);
    
    _learnedFrequencies.clear();
    int threshold = static_cast<int>(static_cast<float>(_learnFrameCount) * 0.3f);
    
    for (size_t i = 0; i < _frequencyHitCount.size(); ++i) {
        if (_frequencyHitCount[i] > threshold) {
            float freq = static_cast<float>(i) * static_cast<float>(_sampleRate) / static_cast<float>(_fftSize);
            _learnedFrequencies.push_back(freq);
        }
    }
    
    _detector.setLearnedFrequencies(_learnedFrequencies);
}

void FeedbackPredictionProcessor::processLearnFrame() {
    const auto& candidates = _detector.getCandidates();
    
    for (const auto& c : candidates) {
        // Se estivermos em modo learn, queremos ignorar feedback óbvio
        // e aprender o espectro "seguro" do ambiente
        if (c.magnitude > -60.0f) { 
            int bin = static_cast<int>(c.frequency * static_cast<float>(_fftSize) / static_cast<float>(_sampleRate));
            if (bin >= 0 && bin < static_cast<int>(_frequencyHitCount.size())) {
                _frequencyHitCount[bin]++;
            }
        }
    }
    
    _learnFrameCount++;
    if (_learnFrameCount >= LEARN_FRAMES) {
        stopLearning();
    }
}

void FeedbackPredictionProcessor::clearLearned() {
    _learnedFrequencies.clear();
    _detector.setLearnedFrequencies({});
    _detector.reset();
}

} // namespace AudioCoPilot
