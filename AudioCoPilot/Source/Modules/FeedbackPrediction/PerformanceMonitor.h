#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <vector>
#include <string>
#include <mutex>

namespace AudioCoPilot {

// Estrutura para métricas de performance
struct PerformanceMetrics {
    juce::int64 timestamp;
    double cpuUsagePercent = 0.0;      // Uso de CPU (%)
    double processingTimeMs = 0.0;     // Tempo de processamento (ms)
    double fftTimeMs = 0.0;           // Tempo de FFT (ms)
    double detectionTimeMs = 0.0;     // Tempo de detecção (ms)
    int framesProcessed = 0;          // Frames processados
    int feedbackEvents = 0;           // Eventos de feedback detectados
    int warningEvents = 0;            // Eventos de warning
    float avgRiskScore = 0.0f;        // Score de risco médio
    float peakRiskScore = 0.0f;       // Score de risco máximo
    float avgLoopGain = 0.0f;         // Loop gain médio
    float peakLoopGain = 0.0f;        // Loop gain máximo
};

// Monitor de performance para o módulo Feedback Prediction
class PerformanceMonitor {
public:
    PerformanceMonitor();
    ~PerformanceMonitor() = default;
    
    // Configuração
    void setEnabled(bool enabled) { _enabled.store(enabled); }
    bool isEnabled() const { return _enabled.load(); }
    void setSamplingInterval(int milliseconds) { _samplingInterval = milliseconds; }
    void setMaxSamples(size_t max) { _maxSamples = max; }
    
    // Início/fim de processamento
    void startProcessing();
    void endProcessing();
    
    // Medições específicas
    void startFFT();
    void endFFT();
    void startDetection();
    void endDetection();
    
    // Eventos
    void recordFeedbackEvent(float riskScore, float loopGain);
    void recordWarningEvent(float riskScore);
    
    // Atualização periódica
    void update();
    
    // Acesso às métricas
    PerformanceMetrics getCurrentMetrics() const;
    std::vector<PerformanceMetrics> getRecentMetrics(size_t maxSamples = 100) const;
    std::vector<PerformanceMetrics> getMetricsInRange(juce::int64 startTime, juce::int64 endTime) const;
    
    // Estatísticas
    PerformanceMetrics getAverageMetrics(size_t lastSeconds = 60) const;
    PerformanceMetrics getPeakMetrics(size_t lastSeconds = 60) const;
    
    // Exportação
    void exportToCSV(const juce::File& file) const;
    void reset();
    
    // Diagnóstico
    bool isOverloaded(double thresholdPercent = 80.0) const;
    std::string getDiagnosticSummary() const;
    
private:
    // Cálculo de uso de CPU (platform-specific)
    double calculateCPUUsage();
    juce::int64 getProcessCPUTime() const;
    juce::int64 getSystemCPUTime() const;
    
    // Métricas atuais
    mutable std::mutex _mutex;
    std::vector<PerformanceMetrics> _metrics;
    size_t _maxSamples = 3600; // 1 hora a 1 sample por segundo
    
    // Estado atual
    juce::int64 _processingStartTime = 0;
    juce::int64 _fftStartTime = 0;
    juce::int64 _detectionStartTime = 0;
    
    // Contadores
    int _framesProcessed = 0;
    int _feedbackEvents = 0;
    int _warningEvents = 0;
    float _totalRiskScore = 0.0f;
    float _peakRiskScore = 0.0f;
    float _totalLoopGain = 0.0f;
    float _peakLoopGain = 0.0f;
    
    // Timers para CPU usage
    juce::int64 _lastCPUTime = 0;
    juce::int64 _lastSystemTime = 0;
    juce::int64 _lastUpdateTime = 0;
    
    // Configuração
    std::atomic<bool> _enabled { true };
    int _samplingInterval = 1000; // ms
    
    // Platform-specific
#ifdef JUCE_MAC
    mutable struct rusage _lastUsage;
#endif
};

} // namespace AudioCoPilot