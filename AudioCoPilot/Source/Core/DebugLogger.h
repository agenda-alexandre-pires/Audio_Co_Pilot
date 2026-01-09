#pragma once

#include <juce_core/juce_core.h>
#include <fstream>
#include <atomic>
#include <mutex>
#include <chrono>

class DebugLogger
{
public:
    static DebugLogger& getInstance()
    {
        static DebugLogger instance;
        return instance;
    }
    
    void log(const juce::String& message, const juce::String& category = "INFO")
    {
        std::lock_guard<std::mutex> lock(_mutex);
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        
        juce::String logLine = "[" + juce::String(ss.str().c_str()) + "] " +
                              "[" + category + "] " + message;
        
        // Log para console
        std::cout << logLine.toStdString() << std::endl;
        
        // Log para arquivo
        if (_logFile.is_open())
        {
            _logFile << logLine.toStdString() << std::endl;
            _logFile.flush();
        }
    }
    
    void logError(const juce::String& message)
    {
        log(message, "ERROR");
    }
    
    void logWarning(const juce::String& message)
    {
        log(message, "WARNING");
    }
    
    void logAudioThread(const juce::String& message)
    {
        log(message, "AUDIO_THREAD");
    }
    
    void logUIThread(const juce::String& message)
    {
        log(message, "UI_THREAD");
    }
    
    void startProfiling(const juce::String& section)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _profileStartTimes[section] = std::chrono::high_resolution_clock::now();
    }
    
    void endProfiling(const juce::String& section)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _profileStartTimes.find(section);
        if (it != _profileStartTimes.end())
        {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - it->second);
            log(juce::String(section) + " took " + juce::String(duration.count()) + "us", "PROFILE");
            _profileStartTimes.erase(it);
        }
    }
    
private:
    DebugLogger()
    {
        // Cria arquivo de log com timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream filename;
        filename << "/tmp/audiocopilot_debug_"
                << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S")
                << ".log";
        
        _logFile.open(filename.str());
        if (_logFile.is_open())
        {
            log("DebugLogger initialized - Log file: " + juce::String(filename.str().c_str()));
        }
        else
        {
            std::cerr << "Failed to open log file: " << filename.str() << std::endl;
        }
    }
    
    ~DebugLogger()
    {
        if (_logFile.is_open())
        {
            log("DebugLogger shutting down");
            _logFile.close();
        }
    }
    
    DebugLogger(const DebugLogger&) = delete;
    DebugLogger& operator=(const DebugLogger&) = delete;
    
    std::ofstream _logFile;
    std::mutex _mutex;
    std::unordered_map<juce::String, std::chrono::high_resolution_clock::time_point> _profileStartTimes;
};

// Macros para facilitar o uso
#define DEBUG_LOG(msg) DebugLogger::getInstance().log(msg)
#define DEBUG_ERROR(msg) DebugLogger::getInstance().logError(msg)
#define DEBUG_WARNING(msg) DebugLogger::getInstance().logWarning(msg)
#define DEBUG_AUDIO(msg) DebugLogger::getInstance().logAudioThread(msg)
#define DEBUG_UI(msg) DebugLogger::getInstance().logUIThread(msg)
#define DEBUG_PROFILE_START(section) DebugLogger::getInstance().startProfiling(section)
#define DEBUG_PROFILE_END(section) DebugLogger::getInstance().endProfiling(section)