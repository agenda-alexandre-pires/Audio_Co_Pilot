#pragma once

#include <juce_core/juce_core.h>
#include "FeedbackLogger.h"
#include "PerformanceMonitor.h"
#include <vector>
#include <string>
#include <memory>

namespace AudioCoPilot {

// Formatos de exportação suportados
enum class ExportFormat {
    CSV,
    JSON,
    XML,
    INFLUXDB_LINE_PROTOCOL,
    PROMETHEUS
};

// Configuração de exportação
struct ExportConfig {
    ExportFormat format = ExportFormat::CSV;
    juce::String destinationPath;
    juce::String influxDBUrl;
    juce::String influxDBToken;
    juce::String influxDBBucket;
    juce::String influxDBOrg;
    juce::String prometheusPushGateway;
    bool exportLogs = true;
    bool exportMetrics = true;
    bool exportEvents = true;
    int exportIntervalSeconds = 60; // Exportação automática a cada 60 segundos
    int maxFileSizeMB = 100;        // Tamanho máximo do arquivo
    int maxFiles = 10;              // Número máximo de arquivos a manter
};

// Sistema de exportação avançada para monitoramento externo
class DataExporter {
public:
    DataExporter();
    ~DataExporter();
    
    // Configuração
    void setConfig(const ExportConfig& config);
    ExportConfig getConfig() const { return _config; }
    
    // Fontes de dados
    void setFeedbackLogger(FeedbackLogger* logger) { _feedbackLogger = logger; }
    void setPerformanceMonitor(PerformanceMonitor* monitor) { _performanceMonitor = monitor; }
    
    // Exportação manual
    bool exportNow();
    bool exportToFile(const juce::File& file, ExportFormat format);
    bool exportToInfluxDB();
    bool exportToPrometheus();
    
    // Exportação automática
    void startAutoExport();
    void stopAutoExport();
    bool isAutoExportRunning() const { return _autoExportRunning; }
    
    // Rotação de arquivos
    void rotateFiles();
    
    // Status
    juce::String getStatus() const;
    juce::int64 getLastExportTime() const { return _lastExportTime; }
    int getExportCount() const { return _exportCount; }
    
private:
    // Métodos de exportação por formato
    juce::String exportToCSV() const;
    juce::String exportToJSON() const;
    juce::String exportToXML() const;
    juce::String exportToInfluxLineProtocol() const;
    juce::String exportToPrometheusFormat() const;
    
    // Helpers
    juce::String escapeCSV(const juce::String& text) const;
    juce::String getTimestampString(juce::int64 timestamp) const;
    juce::String generateFilename() const;
    
    // Thread para exportação automática
    class AutoExportThread : public juce::Thread {
    public:
        AutoExportThread(DataExporter& exporter) 
            : juce::Thread("DataExporter Thread"), _exporter(exporter) {}
        
        void run() override {
            while (!threadShouldExit()) {
                _exporter.exportNow();
                wait(_exporter._config.exportIntervalSeconds * 1000);
            }
        }
        
    private:
        DataExporter& _exporter;
    };
    
    // Dados
    ExportConfig _config;
    FeedbackLogger* _feedbackLogger = nullptr;
    PerformanceMonitor* _performanceMonitor = nullptr;
    
    // Estado
    std::unique_ptr<AutoExportThread> _autoExportThread;
    std::atomic<bool> _autoExportRunning { false };
    juce::int64 _lastExportTime = 0;
    int _exportCount = 0;
    
    // Mutex para thread safety
    mutable juce::CriticalSection _lock;
};

} // namespace AudioCoPilot