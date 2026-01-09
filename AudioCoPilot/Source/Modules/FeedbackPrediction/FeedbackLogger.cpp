#include "FeedbackLogger.h"
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace AudioCoPilot {

FeedbackLogger& FeedbackLogger::getInstance() {
    static FeedbackLogger instance;
    return instance;
}

FeedbackLogger::FeedbackLogger() {
    // Reserva espaço inicial
    _entries.reserve(_maxEntries);
}

void FeedbackLogger::log(LogLevel level, const std::string& module, const std::string& message,
                        float frequency, float riskScore, float magnitude, float loopGain) {
    if (!_enabled.load() || static_cast<int>(level) < _minLevel.load()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(_mutex);
    
    LogEntry entry;
    entry.timestamp = juce::Time::currentTimeMillis();
    entry.level = level;
    entry.module = module;
    entry.message = message;
    entry.frequency = frequency;
    entry.riskScore = riskScore;
    entry.magnitude = magnitude;
    entry.loopGain = loopGain;
    
    _entries.push_back(entry);
    
    // Mantém o tamanho máximo
    if (_entries.size() > _maxEntries) {
        _entries.erase(_entries.begin(), _entries.begin() + (_entries.size() - _maxEntries));
    }
    
    // Log para console em desenvolvimento (apenas para níveis altos)
    if (level >= LogLevel::LOG_WARNING) {
        juce::String logMsg = juce::String(levelToString(level)) + " [" + module + "]: " + message;
        if (frequency > 0.0f) {
            logMsg += " (Freq: " + juce::String(frequency, 1) + " Hz";
            logMsg += ", Risk: " + juce::String(riskScore, 2);
            logMsg += ", Mag: " + juce::String(magnitude, 1) + " dB";
            if (loopGain != 0.0f) {
                logMsg += ", LoopGain: " + juce::String(loopGain, 1) + " dB";
            }
            logMsg += ")";
        }
        DBG(logMsg);
    }
}

void FeedbackLogger::logDebug(const std::string& module, const std::string& message,
                             float frequency, float riskScore, float magnitude, float loopGain) {
    log(LogLevel::LOG_DEBUG, module, message, frequency, riskScore, magnitude, loopGain);
}

void FeedbackLogger::logInfo(const std::string& module, const std::string& message,
                            float frequency, float riskScore, float magnitude, float loopGain) {
    log(LogLevel::LOG_INFO, module, message, frequency, riskScore, magnitude, loopGain);
}

void FeedbackLogger::logWarning(const std::string& module, const std::string& message,
                               float frequency, float riskScore, float magnitude, float loopGain) {
    log(LogLevel::LOG_WARNING, module, message, frequency, riskScore, magnitude, loopGain);
}

void FeedbackLogger::logError(const std::string& module, const std::string& message,
                             float frequency, float riskScore, float magnitude, float loopGain) {
    log(LogLevel::LOG_ERROR, module, message, frequency, riskScore, magnitude, loopGain);
}

void FeedbackLogger::logCritical(const std::string& module, const std::string& message,
                                float frequency, float riskScore, float magnitude, float loopGain) {
    log(LogLevel::LOG_CRITICAL, module, message, frequency, riskScore, magnitude, loopGain);
}

std::vector<LogEntry> FeedbackLogger::getRecentEntries(size_t maxEntries) const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    size_t count = std::min(maxEntries, _entries.size());
    if (count == 0) return {};
    
    return std::vector<LogEntry>(_entries.end() - count, _entries.end());
}

std::vector<LogEntry> FeedbackLogger::getEntriesByLevel(LogLevel level, size_t maxEntries) const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    std::vector<LogEntry> result;
    result.reserve(std::min(maxEntries, _entries.size()));
    
    for (auto it = _entries.rbegin(); it != _entries.rend() && result.size() < maxEntries; ++it) {
        if (it->level == level) {
            result.push_back(*it);
        }
    }
    
    return result;
}

std::vector<LogEntry> FeedbackLogger::getEntriesByModule(const std::string& module, size_t maxEntries) const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    std::vector<LogEntry> result;
    result.reserve(std::min(maxEntries, _entries.size()));
    
    for (auto it = _entries.rbegin(); it != _entries.rend() && result.size() < maxEntries; ++it) {
        if (it->module == module) {
            result.push_back(*it);
        }
    }
    
    return result;
}

void FeedbackLogger::exportToCSV(const juce::File& file) const {
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
    csvFile << "Timestamp,Level,Module,Message,Frequency(Hz),RiskScore,Magnitude(dB),LoopGain(dB)\n";
    
    // Dados
    for (const auto& entry : _entries) {
        csvFile << entry.timestamp << ","
                << levelToString(entry.level) << ","
                << entry.module << ","
                << "\"" << entry.message << "\","
                << std::fixed << std::setprecision(1) << entry.frequency << ","
                << std::setprecision(3) << entry.riskScore << ","
                << std::setprecision(1) << entry.magnitude << ","
                << std::setprecision(1) << entry.loopGain << "\n";
    }
    
    csvFile.close();
    DBG("Exported " << _entries.size() << " log entries to: " << file.getFullPathName());
}

void FeedbackLogger::clear() {
    std::lock_guard<std::mutex> lock(_mutex);
    _entries.clear();
}

size_t FeedbackLogger::getEntriesCountByLevel(LogLevel level) const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    size_t count = 0;
    for (const auto& entry : _entries) {
        if (entry.level == level) {
            count++;
        }
    }
    
    return count;
}

std::string FeedbackLogger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::LOG_DEBUG:    return "DEBUG";
        case LogLevel::LOG_INFO:     return "INFO";
        case LogLevel::LOG_WARNING:  return "WARNING";
        case LogLevel::LOG_ERROR:    return "ERROR";
        case LogLevel::LOG_CRITICAL: return "CRITICAL";
        default:                     return "UNKNOWN";
    }
}

juce::String FeedbackLogger::getTimestampString(juce::int64 timestamp) const {
    juce::Time time(timestamp);
    return time.formatted("%Y-%m-%d %H:%M:%S");
}

} // namespace AudioCoPilot