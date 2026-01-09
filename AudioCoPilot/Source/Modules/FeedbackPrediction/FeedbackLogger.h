#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <vector>
#include <string>
#include <mutex>

namespace AudioCoPilot {

// Níveis de log
enum class LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_CRITICAL
};

// Estrutura para uma entrada de log
struct LogEntry {
    juce::int64 timestamp;
    LogLevel level;
    std::string module;
    std::string message;
    float frequency = 0.0f;
    float riskScore = 0.0f;
    float magnitude = -100.0f;
    float loopGain = 0.0f;
};

// Sistema de logging para debugging em produção
class FeedbackLogger {
public:
    static FeedbackLogger& getInstance();
    
    // Configuração
    void setEnabled(bool enabled) { _enabled.store(enabled); }
    bool isEnabled() const { return _enabled.load(); }
    void setMaxEntries(size_t max) { _maxEntries = max; }
    void setMinLogLevel(LogLevel level) { _minLevel.store(static_cast<int>(level)); }
    
    // Logging
    void log(LogLevel level, const std::string& module, const std::string& message,
             float frequency = 0.0f, float riskScore = 0.0f, 
             float magnitude = -100.0f, float loopGain = 0.0f);
    
    void logDebug(const std::string& module, const std::string& message,
                  float frequency = 0.0f, float riskScore = 0.0f,
                  float magnitude = -100.0f, float loopGain = 0.0f);
    
    void logInfo(const std::string& module, const std::string& message,
                 float frequency = 0.0f, float riskScore = 0.0f,
                 float magnitude = -100.0f, float loopGain = 0.0f);
    
    void logWarning(const std::string& module, const std::string& message,
                    float frequency = 0.0f, float riskScore = 0.0f,
                    float magnitude = -100.0f, float loopGain = 0.0f);
    
    void logError(const std::string& module, const std::string& message,
                  float frequency = 0.0f, float riskScore = 0.0f,
                  float magnitude = -100.0f, float loopGain = 0.0f);
    
    void logCritical(const std::string& module, const std::string& message,
                     float frequency = 0.0f, float riskScore = 0.0f,
                     float magnitude = -100.0f, float loopGain = 0.0f);
    
    // Acesso aos logs
    std::vector<LogEntry> getRecentEntries(size_t maxEntries = 100) const;
    std::vector<LogEntry> getEntriesByLevel(LogLevel level, size_t maxEntries = 100) const;
    std::vector<LogEntry> getEntriesByModule(const std::string& module, size_t maxEntries = 100) const;
    
    // Exportação
    void exportToCSV(const juce::File& file) const;
    void clear();
    
    // Estatísticas
    size_t getTotalEntries() const { return _entries.size(); }
    size_t getEntriesCountByLevel(LogLevel level) const;
    
private:
    FeedbackLogger();
    ~FeedbackLogger() = default;
    
    FeedbackLogger(const FeedbackLogger&) = delete;
    FeedbackLogger& operator=(const FeedbackLogger&) = delete;
    
    std::string levelToString(LogLevel level) const;
    juce::String getTimestampString(juce::int64 timestamp) const;
    
    mutable std::mutex _mutex;
    std::vector<LogEntry> _entries;
    size_t _maxEntries = 1000;
    std::atomic<bool> _enabled { true };
    std::atomic<int> _minLevel { static_cast<int>(LogLevel::LOG_INFO) };
};

// Macros de conveniência
#define FB_LOG_DEBUG(module, message, ...) \
    AudioCoPilot::FeedbackLogger::getInstance().logDebug(module, message, ##__VA_ARGS__)

#define FB_LOG_INFO(module, message, ...) \
    AudioCoPilot::FeedbackLogger::getInstance().logInfo(module, message, ##__VA_ARGS__)

#define FB_LOG_WARNING(module, message, ...) \
    AudioCoPilot::FeedbackLogger::getInstance().logWarning(module, message, ##__VA_ARGS__)

#define FB_LOG_ERROR(module, message, ...) \
    AudioCoPilot::FeedbackLogger::getInstance().logError(module, message, ##__VA_ARGS__)

#define FB_LOG_CRITICAL(module, message, ...) \
    AudioCoPilot::FeedbackLogger::getInstance().logCritical(module, message, ##__VA_ARGS__)

} // namespace AudioCoPilot