#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include <atomic>
#include "FeedbackLogger.h"
#include "PerformanceMonitor.h"

namespace AudioCoPilot {

// Configuração
struct HowlingDetectorConfig {
    static constexpr int FFT_SIZE = 4096;           // Resolução de frequência
    static constexpr int HOP_SIZE = 1024;           // 75% overlap
    static constexpr int MAX_PEAKS = 10;            // Picos a monitorar
    static constexpr int HISTORY_FRAMES = 16;       // Frames no buffer
    static constexpr int NEIGHBOR_BINS = 8;         // Bins para PNPR
    
    // Thresholds base (validados experimentalmente)
    static constexpr float THRESHOLD_PNPR_BASE = 8.0f;   // dB
    static constexpr float THRESHOLD_PHPR_BASE = 6.0f;   // dB
    static constexpr int MIN_PERSISTENCE = 8;        // frames
    static constexpr float MIN_SLOPE = 0.5f;        // dB/frame
    
    // Comparação (Reference vs Measurement)
    static constexpr float THRESHOLD_LOOP_GAIN = 3.0f; // dB (Meas > Ref + 3dB é perigoso)
    
    // Estados
    static constexpr float WARNING_THRESHOLD = 0.6f; // 60% do threshold
    static constexpr float DANGER_THRESHOLD = 0.9f;  // 90% do threshold
    
    // Adaptive Thresholding
    static constexpr float RMS_ALPHA = 0.95f;       // Fator de suavização do RMS
    static constexpr float RMS_MIN = 1e-6f;         // RMS mínimo para evitar divisão por zero
    static constexpr float THRESHOLD_SCALE_LOW = 0.5f;  // Escala para RMS baixo
    static constexpr float THRESHOLD_SCALE_HIGH = 2.0f; // Escala para RMS alto
    static constexpr float RMS_REFERENCE = 0.1f;    // RMS de referência (nível normal)
};

// Resultado por frequência candidata
struct FeedbackCandidate {
    float frequency = 0.0f;        // Hz
    float magnitude = 0.0f;        // dB (Measurement)
    float refMagnitude = -100.0f;  // dB (Reference)
    float loopGain = 0.0f;        // dB (Meas - Ref)
    
    float pnpr = 0.0f;            // dB
    float phpr = 0.0f;            // dB
    int persistence = 0;       // frames
    float slope = 0.0f;           // dB/frame
    
    enum class Status { Safe, Warning, Danger, Feedback };
    Status status = Status::Safe;
    
    float riskScore = 0.0f;       // 0.0 - 1.0
};

class HowlingDetector {
public:
    HowlingDetector();
    ~HowlingDetector() = default;
    
    void prepare(double sampleRate, int fftSize);
    void reset();
    
    // Processa um frame de magnitude FFT (apenas Measurement)
    void processFrame(const float* magnitudeSpectrum, int numBins);
    
    // Processa com Referência (Comparação)
    void processFrameWithReference(const float* measurementSpectrum, 
                                 const float* referenceSpectrum, 
                                 int numBins);
    
    // Retorna candidatos atuais
    const std::array<FeedbackCandidate, HowlingDetectorConfig::MAX_PEAKS>& getCandidates() const;
    
    // Retorna pior status atual
    FeedbackCandidate::Status getWorstStatus() const;
    
    // Frequência com maior risco
    float getMostDangerousFrequency() const;
    
    // Para Learn Stage
    void setLearnedFrequencies(const std::vector<float>& frequencies);
    bool isLearnedFrequency(float freq, float tolerance = 50.0f) const;
    
    // Performance Monitoring
    PerformanceMonitor& getPerformanceMonitor() { return _performanceMonitor; }
    const PerformanceMonitor& getPerformanceMonitor() const { return _performanceMonitor; }
    void enablePerformanceMonitoring(bool enabled) { _performanceMonitor.setEnabled(enabled); }

private:
    double _sampleRate = 48000.0;
    int _fftSize = HowlingDetectorConfig::FFT_SIZE;
    
    // Buffer de histórico de magnitude (Measurement)
    std::vector<std::vector<float>> _magnitudeHistory;
    int _historyIndex = 0;
    
    // Candidatos atuais
    std::array<FeedbackCandidate, HowlingDetectorConfig::MAX_PEAKS> _candidates;
    
    // Persistência por bin
    std::vector<int> _binPersistence;
    
    // Frequências aprendidas (Learn Stage)
    std::vector<float> _learnedFrequencies;
    
    // Buffers temporários pré-alocados (Otimização)
    std::vector<float> _refFrameDB;
    std::vector<int> _peakBins;
    std::vector<float> _slopeValues;
    
    // Adaptive Thresholding
    float _rmsMeasurement = HowlingDetectorConfig::RMS_MIN;
    float _rmsReference = HowlingDetectorConfig::RMS_MIN;
    float _thresholdPNPR = HowlingDetectorConfig::THRESHOLD_PNPR_BASE;
    float _thresholdPHPR = HowlingDetectorConfig::THRESHOLD_PHPR_BASE;
    
    // Performance Monitoring
    PerformanceMonitor _performanceMonitor;
    
    // Funções auxiliares
    void findPeaks(const float* spectrum, int numBins);
    float calculatePNPR(const float* spectrum, int numBins, int peakBin);
    float calculatePHPR(const float* spectrum, int numBins, int peakBin);
    float calculateSlope(int bin);
    float binToFrequency(int bin) const;
    int frequencyToBin(float freq) const;
    float calculateRiskScore(const FeedbackCandidate& candidate);
    FeedbackCandidate::Status determineStatus(float riskScore);
    
    // ISO 226 weighting (audibilidade)
    float getAudibilityWeight(float frequency) const;
    
    // Adaptive Thresholding
    void updateRMS(const float* measurementSpectrum, const float* referenceSpectrum, int numBins);
    void updateAdaptiveThresholds();
    float calculateRMS(const float* spectrum, int numBins) const;
    float getAdaptiveThresholdScale(float rms) const;
};

} // namespace AudioCoPilot
