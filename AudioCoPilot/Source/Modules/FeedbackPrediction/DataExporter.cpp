#include "DataExporter.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <thread>

namespace AudioCoPilot {

DataExporter::DataExporter() {
    // Configuração padrão
    _config.format = ExportFormat::CSV;
    _config.destinationPath = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("AudioCoPilot")
        .getChildFile("exports")
        .getFullPathName();
    
    // Cria diretório se não existir
    juce::File exportDir(_config.destinationPath);
    if (!exportDir.exists()) {
        exportDir.createDirectory();
    }
}

DataExporter::~DataExporter() {
    stopAutoExport();
}

void DataExporter::setConfig(const ExportConfig& config) {
    const juce::ScopedLock lock(_lock);
    _config = config;
    
    // Garante que o diretório existe
    if (!_config.destinationPath.isEmpty()) {
        juce::File exportDir(_config.destinationPath);
        if (!exportDir.exists()) {
            exportDir.createDirectory();
        }
    }
}

bool DataExporter::exportNow() {
    const juce::ScopedLock lock(_lock);
    
    if (!_feedbackLogger && !_performanceMonitor) {
        DBG("DataExporter: No data sources configured");
        return false;
    }
    
    try {
        juce::String data;
        
        switch (_config.format) {
            case ExportFormat::CSV:
                data = exportToCSV();
                break;
            case ExportFormat::JSON:
                data = exportToJSON();
                break;
            case ExportFormat::XML:
                data = exportToXML();
                break;
            case ExportFormat::INFLUXDB_LINE_PROTOCOL:
                return exportToInfluxDB();
            case ExportFormat::PROMETHEUS:
                return exportToPrometheus();
            default:
                data = exportToCSV();
        }
        
        // Salva em arquivo
        juce::String filename = generateFilename();
        juce::File exportFile(juce::File(_config.destinationPath).getChildFile(filename));
        
        if (exportFile.replaceWithText(data)) {
            _lastExportTime = juce::Time::currentTimeMillis();
            _exportCount++;
            
            DBG("DataExporter: Exported " << data.length() << " bytes to " << exportFile.getFullPathName());
            
            // Rotação de arquivos
            if (_exportCount % 5 == 0) { // Rotaciona a cada 5 exportações
                rotateFiles();
            }
            
            return true;
        } else {
            DBG("DataExporter: Failed to write to file: " << exportFile.getFullPathName());
            return false;
        }
        
    } catch (const std::exception& e) {
        DBG("DataExporter: Export failed with error: " << e.what());
        return false;
    }
}

bool DataExporter::exportToFile(const juce::File& file, ExportFormat format) {
    const juce::ScopedLock lock(_lock);
    
    juce::String data;
    
    switch (format) {
        case ExportFormat::CSV:
            data = exportToCSV();
            break;
        case ExportFormat::JSON:
            data = exportToJSON();
            break;
        case ExportFormat::XML:
            data = exportToXML();
            break;
        default:
            data = exportToCSV();
    }
    
    return file.replaceWithText(data);
}

juce::String DataExporter::exportToCSV() const {
    std::stringstream ss;
    
    // Cabeçalho
    ss << "timestamp,type,module,message,frequency,risk_score,magnitude,loop_gain,"
       << "cpu_usage,processing_time,fft_time,detection_time,frames_processed,"
       << "feedback_events,warning_events,avg_risk_score,peak_risk_score,"
       << "avg_loop_gain,peak_loop_gain\n";
    
    // Logs
    if (_feedbackLogger && _config.exportLogs) {
        auto logs = _feedbackLogger->getRecentEntries(1000);
        for (const auto& log : logs) {
            ss << log.timestamp << ",LOG," << log.module << ","
               << escapeCSV(log.message) << ","
               << std::fixed << std::setprecision(1) << log.frequency << ","
               << std::setprecision(3) << log.riskScore << ","
               << std::setprecision(1) << log.magnitude << ","
               << std::setprecision(1) << log.loopGain << ","
               << ",,,,,,,,,,,\n"; // Campos vazios para métricas
        }
    }
    
    // Métricas de performance
    if (_performanceMonitor && _config.exportMetrics) {
        auto metrics = _performanceMonitor->getRecentMetrics(1000);
        for (const auto& metric : metrics) {
            ss << metric.timestamp << ",METRIC,PerformanceMonitor,,"
               << ",,,,,"
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
    }
    
    return ss.str();
}

juce::String DataExporter::exportToJSON() const {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"export_timestamp\": " << juce::Time::currentTimeMillis() << ",\n";
    ss << "  \"data\": [\n";
    
    bool firstEntry = true;
    
    // Logs
    if (_feedbackLogger && _config.exportLogs) {
        auto logs = _feedbackLogger->getRecentEntries(1000);
        for (const auto& log : logs) {
            if (!firstEntry) ss << ",\n";
            firstEntry = false;
            
            ss << "    {\n";
            ss << "      \"type\": \"log\",\n";
            ss << "      \"timestamp\": " << log.timestamp << ",\n";
            ss << "      \"module\": \"" << log.module << "\",\n";
            ss << "      \"message\": \"" << juce::JSON::escapeString(log.message) << "\",\n";
            ss << "      \"frequency\": " << log.frequency << ",\n";
            ss << "      \"risk_score\": " << log.riskScore << ",\n";
            ss << "      \"magnitude\": " << log.magnitude << ",\n";
            ss << "      \"loop_gain\": " << log.loopGain << "\n";
            ss << "    }";
        }
    }
    
    // Métricas
    if (_performanceMonitor && _config.exportMetrics) {
        auto metrics = _performanceMonitor->getRecentMetrics(1000);
        for (const auto& metric : metrics) {
            if (!firstEntry) ss << ",\n";
            firstEntry = false;
            
            ss << "    {\n";
            ss << "      \"type\": \"metric\",\n";
            ss << "      \"timestamp\": " << metric.timestamp << ",\n";
            ss << "      \"cpu_usage_percent\": " << metric.cpuUsagePercent << ",\n";
            ss << "      \"processing_time_ms\": " << metric.processingTimeMs << ",\n";
            ss << "      \"fft_time_ms\": " << metric.fftTimeMs << ",\n";
            ss << "      \"detection_time_ms\": " << metric.detectionTimeMs << ",\n";
            ss << "      \"frames_processed\": " << metric.framesProcessed << ",\n";
            ss << "      \"feedback_events\": " << metric.feedbackEvents << ",\n";
            ss << "      \"warning_events\": " << metric.warningEvents << ",\n";
            ss << "      \"avg_risk_score\": " << metric.avgRiskScore << ",\n";
            ss << "      \"peak_risk_score\": " << metric.peakRiskScore << ",\n";
            ss << "      \"avg_loop_gain\": " << metric.avgLoopGain << ",\n";
            ss << "      \"peak_loop_gain\": " << metric.peakLoopGain << "\n";
            ss << "    }";
        }
    }
    
    ss << "\n  ]\n";
    ss << "}\n";
    
    return ss.str();
}

juce::String DataExporter::exportToXML() const {
    std::stringstream ss;
    
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<audiocopilot_export timestamp=\"" << juce::Time::currentTimeMillis() << "\">\n";
    
    // Logs
    if (_feedbackLogger && _config.exportLogs) {
        ss << "  <logs>\n";
        auto logs = _feedbackLogger->getRecentEntries(1000);
        for (const auto& log : logs) {
            ss << "    <log>\n";
            ss << "      <timestamp>" << log.timestamp << "</timestamp>\n";
            ss << "      <module>" << log.module << "</module>\n";
            ss << "      <message><![CDATA[" << log.message << "]]></message>\n";
            ss << "      <frequency>" << log.frequency << "</frequency>\n";
            ss << "      <risk_score>" << log.riskScore << "</risk_score>\n";
            ss << "      <magnitude>" << log.magnitude << "</magnitude>\n";
            ss << "      <loop_gain>" << log.loopGain << "</loop_gain>\n";
            ss << "    </log>\n";
        }
        ss << "  </logs>\n";
    }
    
    // Métricas
    if (_performanceMonitor && _config.exportMetrics) {
        ss << "  <metrics>\n";
        auto metrics = _performanceMonitor->getRecentMetrics(1000);
        for (const auto& metric : metrics) {
            ss << "    <metric>\n";
            ss << "      <timestamp>" << metric.timestamp << "</timestamp>\n";
            ss << "      <cpu_usage_percent>" << metric.cpuUsagePercent << "</cpu_usage_percent>\n";
            ss << "      <processing_time_ms>" << metric.processingTimeMs << "</processing_time_ms>\n";
            ss << "      <fft_time_ms>" << metric.fftTimeMs << "</fft_time_ms>\n";
            ss << "      <detection_time_ms>" << metric.detectionTimeMs << "</detection_time_ms>\n";
            ss << "      <frames_processed>" << metric.framesProcessed << "</frames_processed>\n";
            ss << "      <feedback_events>" << metric.feedbackEvents << "</feedback_events>\n";
            ss << "      <warning_events>" << metric.warningEvents << "</warning_events>\n";
            ss << "      <avg_risk_score>" << metric.avgRiskScore << "</avg_risk_score>\n";
            ss << "      <peak_risk_score>" << metric.peakRiskScore << "</peak_risk_score>\n";
            ss << "      <avg_loop_gain>" << metric.avgLoopGain << "</avg_loop_gain>\n";
            ss << "      <peak_loop_gain>" << metric.peakLoopGain << "</peak_loop_gain>\n";
            ss << "    </metric>\n";
        }
        ss << "  </metrics>\n";
    }
    
    ss << "</audiocopilot_export>\n";
    
    return ss.str();
}

bool DataExporter::exportToInfluxDB() {
    if (_config.influxDBUrl.isEmpty() || _config.influxDBToken.isEmpty()) {
        DBG("DataExporter: InfluxDB configuration missing");
        return false;
    }
    
    // Implementação simplificada - em produção, usar biblioteca HTTP
    DBG("DataExporter: InfluxDB export not fully implemented");
    return false;
}

bool DataExporter::exportToPrometheus() {
    if (_config.prometheusPushGateway.isEmpty()) {
        DBG("DataExporter: Prometheus configuration missing");
        return false;
    }
    
    // Implementação simplificada - em produção, usar biblioteca HTTP
    DBG("DataExporter: Prometheus export not fully implemented");
    return false;
}

void DataExporter::startAutoExport() {
    stopAutoExport();
    
    _autoExportThread = std::make_unique<AutoExportThread>(*this);
    _autoExportRunning = true;
    _autoExportThread->startThread();
    
    DBG("DataExporter: Auto export started with interval " << _config.exportIntervalSeconds << " seconds");
}

void DataExporter::stopAutoExport() {
    if (_autoExportThread) {
        _autoExportRunning = false;
        _autoExportThread->stopThread(5000); // Timeout de 5 segundos
        _autoExportThread.reset();
    }
}

void DataExporter::rotateFiles() {
    juce::File exportDir(_config.destinationPath);
    if (!exportDir.exists()) return;
    
    juce::Array<juce::File> files;
    exportDir.findChildFiles(files, juce::File::findFiles, false, "*.csv;*.json;*.xml");
    
    // Ordena por data de modificação (mais antigo primeiro)
    files.sort();
    
    // Remove arquivos antigos se exceder o limite
    while (files.size() > _config.maxFiles) {
        files[0].deleteFile();
        files.remove(0);
    }
    
    // Verifica tamanho dos arquivos
    for (auto& file : files) {
        if (file.getSize() > _config.maxFileSizeMB * 1024 * 1024) {
            // Arquivo muito grande, remove
            file.deleteFile();
        }
    }
}

juce::String DataExporter::getStatus() const {
    std::stringstream ss;
    
    ss << "DataExporter Status:\n";
    ss << "  Auto Export: " << (_autoExportRunning ? "Running" : "Stopped") << "\n";
    ss << "  Format: ";
    
    switch (_config.format) {
        case ExportFormat::CSV: ss << "CSV"; break;
        case ExportFormat::JSON: ss << "JSON"; break;
        case ExportFormat::XML: ss << "XML"; break;
        case ExportFormat::INFLUXDB_LINE_PROTOCOL: ss << "InfluxDB"; break;
        case ExportFormat::PROMETHEUS: ss << "Prometheus"; break;
    }
    
    ss << "\n";
    ss << "  Destination: " << _config.destinationPath.toStdString() << "\n";
    ss << "  Export Count: " << _exportCount << "\n";
    ss << "  Last Export: ";
    
    if (_lastExportTime > 0) {
        juce::Time lastTime(_lastExportTime);
        ss << lastTime.formatted("%Y-%m-%d %H:%M:%S").toStdString();
    } else {
        ss << "Never";
    }
    
    ss << "\n";
    ss << "  Data Sources: ";
    
    if (_feedbackLogger) ss << "Logger ";
    if (_performanceMonitor) ss << "Monitor";
    if (!_feedbackLogger && !_performanceMonitor) ss << "None";
    
    return ss.str();
}

juce::String DataExporter::escapeCSV(const juce::String& text) const {
    juce::String escaped = text;
    escaped = escaped.replace("\"", "\"\""); // Escape aspas duplas
    if (escaped.containsAnyOf(",\"\n\r")) {
        escaped = "\"" + escaped + "\""; // Coloca entre aspas se contiver caracteres especiais
    }
    return escaped;
}

juce::String DataExporter::getTimestampString(juce::int64 timestamp) const {
    juce::Time time(timestamp);
    return time.formatted("%Y%m%d_%H%M%S");
}

juce::String DataExporter::generateFilename() const {
    juce::String extension;
    
    switch (_config.format) {
        case ExportFormat::CSV: extension = ".csv"; break;
        case ExportFormat::JSON: extension = ".json"; break;
        case ExportFormat::XML: extension = ".xml"; break;
        default: extension = ".txt";
    }
    
    return "audiocopilot_export_" + getTimestampString(juce::Time::currentTimeMillis()) + extension;
}

} // namespace AudioCoPilot