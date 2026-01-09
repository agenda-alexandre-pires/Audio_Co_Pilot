#include "PerformanceMonitor.h"
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <thread>

#ifdef JUCE_MAC
#include <sys/resource.h>
#include <sys/time.h>
#include <mach/mach.h>
#endif

namespace AudioCoPilot {

PerformanceMonitor::PerformanceMonitor() {
    _metrics.reserve(_maxSamples);
    
#ifdef JUCE_MAC
    getrusage(RUSAGE_SELF, &_lastUsage);
#endif
    
    _lastUpdateTime = juce::Time::currentTimeMillis();
}

void PerformanceMonitor::startProcessing() {
    if (!_enabled.load()) return;
    
    std::lock_guard<std::mutex> lock(_mutex);
    _processingStartTime = juce::Time::getMillisecondCounterHiRes();
    _framesProcessed++;
}

void PerformanceMonitor::endProcessing() {
    if (!_enabled.load() || _processingStartTime == 0) return;
    
    std::lock_guard<std::mutex> lock(_mutex);
    
    // Atualiza métricas acumuladas
    _processingStartTime = 0;
    
    // Verifica se é hora de fazer sampling
    juce::int64 currentTime = juce::Time::currentTimeMillis();
    if (currentTime - _lastUpdateTime >= _samplingInterval) {
        update();
    }
}

void PerformanceMonitor::startFFT() {
    if (!_enabled.load()) return;
    _fftStartTime = juce::Time::getMillisecondCounterHiRes();
}

void PerformanceMonitor::endFFT() {
    if (!_enabled.load() || _fftStartTime == 0) return;
    
    std::lock_guard<std::mutex> lock(_mutex);
    // Podemos armazenar isso se necessário para análise detalhada
    _fftStartTime = 0;
}

void PerformanceMonitor::startDetection() {
    if (!_enabled.load()) return;
    _detectionStartTime = juce::Time::getMillisecondCounterHiRes();
}

void PerformanceMonitor::endDetection() {
    if (!_enabled.load() || _detectionStartTime == 0) return;
    
    std::lock_guard<std::mutex> lock(_mutex);
    // Podemos armazenar isso se necessário para análise detalhada
    _detectionStartTime = 0;
}

void PerformanceMonitor::recordFeedbackEvent(float riskScore, float loopGain) {
    if (!_enabled.load()) return;
    
    std::lock_guard<std::mutex> lock(_mutex);
    _feedbackEvents++;
    _totalRiskScore += riskScore;
    _totalLoopGain += loopGain;
    
    if (riskScore > _peakRiskScore) _peakRiskScore = riskScore;
    if (loopGain > _peakLoopGain) _peakLoopGain = loopGain;
}

void PerformanceMonitor::recordWarningEvent(float riskScore) {
    if (!_enabled.load()) return;
    
    std::lock_guard<std::mutex> lock(_mutex);
    _warningEvents++;
    _totalRiskScore += riskScore;
    
    if (riskScore > _peakRiskScore) _peakRiskScore = riskScore;
}

#ifdef JUCE_MAC
juce::int64 PerformanceMonitor::getProcessCPUTime() const {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    
    // Converte timeval para microssegundos
    juce::int64 userTime = usage.ru_utime.tv_sec * 1000000LL + usage.ru_utime.tv_usec;
    juce::int64 systemTime = usage.ru_stime.tv_sec * 1000000LL + usage.ru_stime.tv_usec;
    
    return userTime + systemTime;
}

juce::int64 PerformanceMonitor::getSystemCPUTime() const {
    host_cpu_load_info_data_t cpuInfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                       (host_info_t)&cpuInfo, &count) != KERN_SUCCESS) {
        return 0;
    }
    
    // Soma todos os tempos de CPU (user + system + nice + idle)
    juce::int64 totalTicks = cpuInfo.cpu_ticks[CPU_STATE_USER] +
                            cpuInfo.cpu_ticks[CPU_STATE_SYSTEM] +
                            cpuInfo.cpu_ticks[CPU_STATE_NICE] +
                            cpuInfo.cpu_ticks[CPU_STATE_IDLE];
    
    return totalTicks;
}
#endif

double PerformanceMonitor::calculateCPUUsage() {
#ifdef JUCE_MAC
    juce::int64 currentProcessTime = getProcessCPUTime();
    juce::int64 currentSystemTime = getSystemCPUTime();
    
    if (_lastCPUTime == 0 || _lastSystemTime == 0) {
        _lastCPUTime = currentProcessTime;
        _lastSystemTime = currentSystemTime;
        return 0.0;
    }
    
    juce::int64 processDiff = currentProcessTime - _lastCPUTime;
    juce::int64 systemDiff = currentSystemTime - _lastSystemTime;
    
    if (systemDiff == 0) return 0.0;
    
    double cpuUsage = (static_cast<double>(processDiff) / static_cast<double>(systemDiff)) * 100.0;
    
    // Atualiza para próxima medição
    _lastCPUTime = currentProcessTime;
    _lastSystemTime = currentSystemTime;
    
    return std::min(cpuUsage, 100.0);
#else
    // Fallback para outras plataformas
    return 0.0;
#endif
}

void PerformanceMonitor::update() {
    if (!_enabled.load()) return;
    
    std::lock_guard<std::mutex> lock(_mutex);
    
    PerformanceMetrics metrics;
    metrics.timestamp = juce::Time::currentTimeMillis();
    metrics.cpuUsagePercent = calculateCPUUsage();
    
    // Calcula métricas baseadas nos contadores
    if (_framesProcessed > 0) {
        metrics.avgRiskScore = _totalRiskScore / static_cast<float>(_framesProcessed);
        metrics.avgLoopGain = _totalLoopGain / static_cast<float>(_framesProcessed);
    }
    
    metrics.peakRiskScore = _peakRiskScore;
    metrics.peakLoopGain = _peakLoopGain;
    metrics.framesProcessed = _framesProcessed;
    metrics.feedbackEvents = _feedbackEvents;
    metrics.warningEvents = _warningEvents;
    
    // Adiciona às métricas históricas
    _metrics.push_back(metrics);
    
    // Mantém tamanho máximo
    if (_metrics.size() > _maxSamples) {
        _metrics.erase(_metrics.begin(), _metrics.begin() + (_metrics.size() - _maxSamples));
    }
    
    // Reseta contadores para próximo intervalo
    _framesProcessed = 0;
    _feedbackEvents = 0;
    _warningEvents = 0;
    _totalRiskScore = 0.0f;
    _peakRiskScore = 0.0f;
    _totalLoopGain = 0.0f;
    _peakLoopGain = 0.0f;
    
    _lastUpdateTime = metrics.timestamp;
}

PerformanceMetrics PerformanceMonitor::getCurrentMetrics() const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    if (_metrics.empty()) {
        return PerformanceMetrics{};
    }
    
    return _metrics.back();
}

std::vector<PerformanceMetrics> PerformanceMonitor::getRecentMetrics(size_t maxSamples) const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    size_t count = std::min(maxSamples, _metrics.size());
    if (count == 0) return {};
    
    return std::vector<PerformanceMetrics>(_metrics.end() - count, _metrics.end());
}

std::vector<PerformanceMetrics> PerformanceMonitor::getMetricsInRange(juce::int64 startTime, juce::int64 endTime) const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    std::vector<PerformanceMetrics> result;
    for (const auto& metric : _metrics) {
        if (metric.timestamp >= startTime && metric.timestamp <= endTime) {
            result.push_back(metric);
        }
    }
    
    return result;
}

PerformanceMetrics PerformanceMonitor::getAverageMetrics(size_t lastSeconds) const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    if (_metrics.empty()) return PerformanceMetrics{};
    
    juce::int64 cutoffTime = juce::Time::currentTimeMillis() - (lastSeconds * 1000);
    
    PerformanceMetrics avg;
    int count = 0;
    
    for (auto it = _metrics.rbegin(); it != _metrics.rend(); ++it) {
        if (it->timestamp < cutoffTime) break;
        
        avg.cpuUsagePercent += it->cpuUsagePercent;
        avg.processingTimeMs += it->processingTimeMs;
        avg.fftTimeMs += it->fftTimeMs;
        avg.detectionTimeMs += it->detectionTimeMs;
        avg.framesProcessed += it->framesProcessed;
        avg.feedbackEvents += it->feedbackEvents;
        avg.warningEvents += it->warningEvents;
        avg.avgRiskScore += it->avgRiskScore;
        avg.peakRiskScore = std::max(avg.peakRiskScore, it->peakRiskScore);
        avg.avgLoopGain += it->avgLoopGain;
        avg.peakLoopGain = std::max(avg.peakLoopGain, it->peakLoopGain);
        
        count++;
    }
    
    if (count > 0) {
        avg.cpuUsagePercent /= count;
        avg.processingTimeMs /= count;
        avg.fftTimeMs /= count;
        avg.detectionTimeMs /= count;
        avg.avgRiskScore /= count;
        avg.avgLoopGain /= count;
        avg.framesProcessed /= count;
        avg.feedbackEvents /= count;
        avg.warningEvents /= count;
    }
    
    return avg;
}

PerformanceMetrics PerformanceMonitor::getPeakMetrics(size_t lastSeconds) const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    if (_metrics.empty()) return PerformanceMetrics{};
    
    juce::int64 cutoffTime = juce::Time::currentTimeMillis() - (lastSeconds * 1000);
    
    PerformanceMetrics peak;
    
    for (auto it = _metrics.rbegin(); it != _metrics.rend(); ++it) {
        if (it->timestamp < cutoffTime) break;
        
        peak.cpuUsagePercent = std::max(peak.cpuUsagePercent, it->cpuUsagePercent);
        peak.processingTimeMs = std::max(peak.processingTimeMs, it->processingTimeMs);
        peak.fftTimeMs = std::max(peak.fftTimeMs, it->fftTimeMs);
        peak.detectionTimeMs = std::max(peak.detectionTimeMs, it->detectionTimeMs);
        peak.framesProcessed = std::max(peak.framesProcessed, it->framesProcessed);
        peak.feedbackEvents = std::max(peak.feedbackEvents, it->feedbackEvents);
        peak.warningEvents = std::max(peak.warningEvents, it->warningEvents);
        peak.avgRiskScore = std::max(peak.avgRiskScore, it->avgRiskScore);
        peak.peakRiskScore = std::max(peak.peakRiskScore, it->peakRiskScore);
        peak.avgLoopGain = std::max(peak.avgLoopGain, it->avgLoopGain);
        peak.peakLoopGain = std::max(peak.peakLoopGain, it->peakLoopGain);
    }
    
    return peak;
}

void PerformanceMonitor::exportToCSV(const juce::File& file) const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    if (file.exists()) {
        file.deleteFile();
    }
    
    std::ofstream csvFile(file.getFullPathName().toStdString());
    if (!csvFile.is_open()) {
        DBG("Failed to open CSV file for writing: " << file.getFullPathName());
        return;
    }
    
    // Cabeçalho
    csvFile << "Timestamp,CPUUsage(%),ProcessingTime(ms),FFTTime(ms),DetectionTime(ms),"
            << "FramesProcessed,FeedbackEvents,WarningEvents,AvgRiskScore,PeakRiskScore,"
            << "AvgLoopGain(dB),PeakLoopGain(dB)\n";
    
    // Dados
    for (const auto& metric : _metrics) {
        juce::Time time(metric.timestamp);
        csvFile << metric.timestamp << ","
                << std::fixed << std::setprecision(2) << metric.cpuUsagePercent << ","
                << std::setprecision(3) << metric.processingTimeMs << ","
                << std::setprecision(3) << metric.fftTimeMs << ","
                << std::setprecision(3) << metric.detectionTimeMs << ","
                << metric.framesProcessed << ","
                << metric.feedbackEvents << ","
                << metric.warningEvents << ","
                << std::setprecision(3) << metric.avgRiskScore << ","
                << std::setprecision(3) << metric.peakRiskScore << ","
                << std::setprecision(1) << metric.avgLoopGain << ","
                << std::setprecision(1) << metric.peakLoopGain << "\n";
    }
    
    csvFile.close();
    DBG("Exported " << _metrics.size() << " performance metrics to: " << file.getFullPathName());
}

void PerformanceMonitor::reset() {
    std::lock_guard<std::mutex> lock(_mutex);
    _metrics.clear();
    _framesProcessed = 0;
    _feedbackEvents = 0;
    _warningEvents = 0;
    _totalRiskScore = 0.0f;
    _peakRiskScore = 0.0f;
    _totalLoopGain = 0.0f;
    _peakLoopGain = 0.0f;
    _lastCPUTime = 0;
    _lastSystemTime = 0;
    _lastUpdateTime = juce::Time::currentTimeMillis();
}

bool PerformanceMonitor::isOverloaded(double thresholdPercent) const {
    auto metrics = getCurrentMetrics();
    return metrics.cpuUsagePercent > thresholdPercent;
}

std::string PerformanceMonitor::getDiagnosticSummary() const {
    auto current = getCurrentMetrics();
    auto avgLastMinute = getAverageMetrics(60);
    auto peakLastMinute = getPeakMetrics(60);
    
    std::stringstream ss;
    ss << "=== Performance Diagnostics ===\n";
    ss << "Current CPU Usage: " << std::fixed << std::setprecision(1) << current.cpuUsagePercent << "%\n";
    ss << "Last Minute Average: " << avgLastMinute.cpuUsagePercent << "%\n";
    ss << "Last Minute Peak: " << peakLastMinute.cpuUsagePercent << "%\n";
    ss << "Frames Processed: " << current.framesProcessed << "\n";
    ss << "Feedback Events: " << current.feedbackEvents << "\n";
    ss << "Warning Events: " << current.warningEvents << "\n";
    ss << "Avg Risk Score: " << std::setprecision(3) << current.avgRiskScore << "\n";
    ss << "Peak Risk Score: " << current.peakRiskScore << "\n";
    ss << "Status: " << (isOverloaded() ? "OVERLOADED" : "OK") << "\n";
    
    return ss.str();
}

} // namespace AudioCoPilot