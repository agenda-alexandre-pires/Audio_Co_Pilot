#include "DynamicConfig.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace AudioCoPilot {

DynamicConfig& DynamicConfig::getInstance() {
    static DynamicConfig instance;
    return instance;
}

DynamicConfig::DynamicConfig() {
    initializeDefaultParameters();
}

void DynamicConfig::initializeDefaultParameters() {
    // Limpa parâmetros existentes
    _parameters.clear();
    
    // Grupo: Detecção
    {
        ConfigParameter param(PARAM_THRESHOLD_PNPR, "PNPR Threshold", ParameterType::FLOAT);
        param.description = "Peak-to-Neighbor Power Ratio threshold (dB)";
        param.floatValue = 8.0f;
        param.defaultValue = 8.0f;
        param.minValue = 1.0f;
        param.maxValue = 20.0f;
        registerParameter(param);
    }
    
    {
        ConfigParameter param(PARAM_THRESHOLD_PHPR, "PHPR Threshold", ParameterType::FLOAT);
        param.description = "Peak-to-Harmonic Power Ratio threshold (dB)";
        param.floatValue = 6.0f;
        param.defaultValue = 6.0f;
        param.minValue = 1.0f;
        param.maxValue = 20.0f;
        registerParameter(param);
    }
    
    {
        ConfigParameter param(PARAM_MIN_PERSISTENCE, "Min Persistence", ParameterType::INT);
        param.description = "Minimum frames for frequency persistence";
        param.intValue = 8;
        param.defaultValue = 8.0f;
        param.minValue = 1;
        param.maxValue = 50;
        registerParameter(param);
    }
    
    {
        ConfigParameter param(PARAM_MIN_SLOPE, "Min Slope", ParameterType::FLOAT);
        param.description = "Minimum slope for feedback detection (dB/frame)";
        param.floatValue = 0.5f;
        param.defaultValue = 0.5f;
        param.minValue = 0.1f;
        param.maxValue = 5.0f;
        registerParameter(param);
    }
    
    {
        ConfigParameter param(PARAM_LOOP_GAIN_THRESHOLD, "Loop Gain Threshold", ParameterType::FLOAT);
        param.description = "Loop gain threshold for feedback detection (dB)";
        param.floatValue = 3.0f;
        param.defaultValue = 3.0f;
        param.minValue = 0.0f;
        param.maxValue = 20.0f;
        registerParameter(param);
    }
    
    // Grupo: Adaptive Thresholding
    {
        ConfigParameter param(PARAM_ENABLE_ADAPTIVE, "Enable Adaptive Thresholding", ParameterType::BOOL);
        param.description = "Enable adaptive thresholding based on RMS";
        param.boolValue = true;
        param.defaultValue = 1.0f; // true
        registerParameter(param);
    }
    
    {
        ConfigParameter param(PARAM_RMS_ALPHA, "RMS Alpha", ParameterType::FLOAT);
        param.description = "RMS smoothing factor (0.0-1.0)";
        param.floatValue = 0.95f;
        param.defaultValue = 0.95f;
        param.minValue = 0.5f;
        param.maxValue = 0.99f;
        registerParameter(param);
    }
    
    // Grupo: Learning
    {
        ConfigParameter param(PARAM_ENABLE_LEARNING, "Enable Learning", ParameterType::BOOL);
        param.description = "Enable frequency learning stage";
        param.boolValue = true;
        param.defaultValue = 1.0f; // true
        registerParameter(param);
    }
    
    {
        ConfigParameter param(PARAM_LEARN_FRAMES, "Learn Frames", ParameterType::INT);
        param.description = "Number of frames for learning stage";
        param.intValue = 500;
        param.defaultValue = 500.0f;
        param.minValue = 10;
        param.maxValue = 5000;
        registerParameter(param);
    }
    
    // Grupo: Logging
    {
        ConfigParameter param(PARAM_ENABLE_LOGGING, "Enable Logging", ParameterType::BOOL);
        param.description = "Enable system logging";
        param.boolValue = true;
        param.defaultValue = 1.0f; // true
        registerParameter(param);
    }
    
    {
        ConfigParameter param(PARAM_LOG_LEVEL, "Log Level", ParameterType::CHOICE);
        param.description = "Minimum log level to record";
        param.stringValue = "INFO";
        param.defaultValue = 0.0f;
        param.choices = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
        registerParameter(param);
    }
    
    // Grupo: Performance Monitoring
    {
        ConfigParameter param(PARAM_ENABLE_PERF_MONITOR, "Enable Performance Monitor", ParameterType::BOOL);
        param.description = "Enable performance monitoring";
        param.boolValue = true;
        param.defaultValue = 1.0f; // true
        registerParameter(param);
    }
    
    {
        ConfigParameter param(PARAM_PERF_SAMPLING_INTERVAL, "Performance Sampling Interval", ParameterType::INT);
        param.description = "Performance sampling interval (ms)";
        param.intValue = 1000;
        param.defaultValue = 1000.0f;
        param.minValue = 100;
        param.maxValue = 10000;
        registerParameter(param);
    }
    
    // Grupo: Export
    {
        ConfigParameter param(PARAM_ENABLE_AUTO_EXPORT, "Enable Auto Export", ParameterType::BOOL);
        param.description = "Enable automatic data export";
        param.boolValue = false;
        param.defaultValue = 0.0f; // false
        registerParameter(param);
    }
    
    {
        ConfigParameter param(PARAM_EXPORT_INTERVAL, "Export Interval", ParameterType::INT);
        param.description = "Auto export interval (seconds)";
        param.intValue = 60;
        param.defaultValue = 60.0f;
        param.minValue = 10;
        param.maxValue = 3600;
        registerParameter(param);
    }
    
    {
        ConfigParameter param(PARAM_EXPORT_FORMAT, "Export Format", ParameterType::CHOICE);
        param.description = "Data export format";
        param.stringValue = "CSV";
        param.defaultValue = 0.0f;
        param.choices = {"CSV", "JSON", "XML"};
        registerParameter(param);
    }
    
    // Cria grupos
    createGroup("Detection", {
        PARAM_THRESHOLD_PNPR,
        PARAM_THRESHOLD_PHPR,
        PARAM_MIN_PERSISTENCE,
        PARAM_MIN_SLOPE,
        PARAM_LOOP_GAIN_THRESHOLD
    });
    
    createGroup("Adaptive", {
        PARAM_ENABLE_ADAPTIVE,
        PARAM_RMS_ALPHA
    });
    
    createGroup("Learning", {
        PARAM_ENABLE_LEARNING,
        PARAM_LEARN_FRAMES
    });
    
    createGroup("Logging", {
        PARAM_ENABLE_LOGGING,
        PARAM_LOG_LEVEL
    });
    
    createGroup("Performance", {
        PARAM_ENABLE_PERF_MONITOR,
        PARAM_PERF_SAMPLING_INTERVAL
    });
    
    createGroup("Export", {
        PARAM_ENABLE_AUTO_EXPORT,
        PARAM_EXPORT_INTERVAL,
        PARAM_EXPORT_FORMAT
    });
}

void DynamicConfig::registerParameter(const ConfigParameter& param) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    // Verifica se já existe
    auto it = std::find_if(_parameters.begin(), _parameters.end(),
                          [&](const ConfigParameter& p) { return p.id == param.id; });
    
    if (it != _parameters.end()) {
        *it = param; // Atualiza
    } else {
        _parameters.push_back(param); // Adiciona novo
    }
}

bool DynamicConfig::updateParameter(const std::string& id, const juce::var& value) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto it = std::find_if(_parameters.begin(), _parameters.end(),
                          [&](const ConfigParameter& p) { return p.id == id; });
    
    if (it == _parameters.end()) {
        DBG("DynamicConfig: Parameter not found: " << id);
        return false;
    }
    
    if (it->isReadOnly) {
        DBG("DynamicConfig: Parameter is read-only: " << id);
        return false;
    }
    
    // Valida o valor
    if (!validateParameterValue(*it, value)) {
        DBG("DynamicConfig: Invalid value for parameter: " << id);
        return false;
    }
    
    // Atualiza o valor
    switch (it->type) {
        case ParameterType::FLOAT:
            it->floatValue = static_cast<float>(value);
            break;
        case ParameterType::INT:
            it->intValue = static_cast<int>(value);
            break;
        case ParameterType::BOOL:
            it->boolValue = static_cast<bool>(value);
            break;
        case ParameterType::STRING:
            it->stringValue = value.toString().toStdString();
            break;
        case ParameterType::CHOICE:
            it->stringValue = value.toString().toStdString();
            // Verifica se é uma escolha válida
            if (std::find(it->choices.begin(), it->choices.end(), it->stringValue) == it->choices.end()) {
                DBG("DynamicConfig: Invalid choice for parameter: " << id);
                return false;
            }
            break;
    }
    
    // Chama callback se existir
    if (it->onChangeCallback) {
        it->onChangeCallback(*it);
    }
    
    // Notifica listeners
    notifyListeners(id);
    
    DBG("DynamicConfig: Updated parameter " << id << " to " << value.toString());
    return true;
}

ConfigParameter DynamicConfig::getParameter(const std::string& id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto it = std::find_if(_parameters.begin(), _parameters.end(),
                          [&](const ConfigParameter& p) { return p.id == id; });
    
    if (it != _parameters.end()) {
        return *it;
    }
    
    return ConfigParameter("", "", ParameterType::FLOAT);
}

std::vector<ConfigParameter> DynamicConfig::getAllParameters() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _parameters;
}

void DynamicConfig::createGroup(const std::string& groupName, const std::vector<std::string>& paramIds) {
    std::lock_guard<std::mutex> lock(_mutex);
    _groups[groupName] = paramIds;
}

std::vector<std::string> DynamicConfig::getParametersInGroup(const std::string& groupName) const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto it = _groups.find(groupName);
    if (it != _groups.end()) {
        return it->second;
    }
    
    return {};
}

void DynamicConfig::loadFromFile(const juce::File& file) {
    if (!file.existsAsFile()) {
        DBG("DynamicConfig: Config file not found: " << file.getFullPathName());
        return;
    }
    
    juce::var configData;
    if (juce::JSON::parse(file.loadFileAsString(), configData).wasOk()) {
        if (configData.isObject()) {
            auto* obj = configData.getDynamicObject();
            if (obj) {
                auto& properties = obj->getProperties();
                
                for (auto& property : properties) {
                    updateParameter(property.name.toString().toStdString(), property.value);
                }
                
                DBG("DynamicConfig: Loaded configuration from " << file.getFullPathName());
            }
        }
    } else {
        DBG("DynamicConfig: Failed to parse config file: " << file.getFullPathName());
    }
}

void DynamicConfig::saveToFile(const juce::File& file) const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    juce::DynamicObject::Ptr configObj = new juce::DynamicObject();
    
    for (const auto& param : _parameters) {
        juce::var value;
        
        switch (param.type) {
            case ParameterType::FLOAT:
                value = param.floatValue;
                break;
            case ParameterType::INT:
                value = param.intValue;
                break;
            case ParameterType::BOOL:
                value = param.boolValue;
                break;
            case ParameterType::STRING:
            case ParameterType::CHOICE:
                value = juce::var(param.stringValue);
                break;
        }
        
        configObj->setProperty(juce::Identifier(param.id), value);
    }
    
    juce::File configDir = file.getParentDirectory();
    if (!configDir.exists()) {
        configDir.createDirectory();
    }
    
    file.replaceWithText(juce::JSON::toString(juce::var(configObj.get()), true));
    DBG("DynamicConfig: Saved configuration to " << file.getFullPathName());
}

void DynamicConfig::loadDefaults() {
    std::lock_guard<std::mutex> lock(_mutex);
    
    for (auto& param : _parameters) {
        switch (param.type) {
            case ParameterType::FLOAT:
                param.floatValue = param.defaultValue;
                break;
            case ParameterType::INT:
                param.intValue = static_cast<int>(param.defaultValue);
                break;
            case ParameterType::BOOL:
                param.boolValue = param.defaultValue > 0.5f;
                break;
            case ParameterType::STRING:
            case ParameterType::CHOICE:
                // Mantém o valor atual se não houver default específico
                break;
        }
        
        // Chama callback se existir
        if (param.onChangeCallback) {
            param.onChangeCallback(param);
        }
        
        // Notifica listeners
        notifyListeners(param.id);
    }
    
    DBG("DynamicConfig: Loaded default configuration");
}

void DynamicConfig::addListener(std::function<void(const std::string& paramId)> listener) {
    std::lock_guard<std::mutex> lock(_mutex);
    _listeners.push_back(listener);
}

void DynamicConfig::removeListener(std::function<void(const std::string& paramId)> listener) {
    std::lock_guard<std::mutex> lock(_mutex);
    // Implementação simplificada - em produção, usaríamos uma abordagem diferente
    // para comparar std::function
    _listeners.clear();
}

float DynamicConfig::getFloat(const std::string& id, float defaultValue) const {
    auto param = getParameter(id);
    if (param.id.empty() || param.type != ParameterType::FLOAT) {
        return defaultValue;
    }
    return param.floatValue;
}

int DynamicConfig::getInt(const std::string& id, int defaultValue) const {
    auto param = getParameter(id);
    if (param.id.empty() || param.type != ParameterType::INT) {
        return defaultValue;
    }
    return param.intValue;
}

bool DynamicConfig::getBool(const std::string& id, bool defaultValue) const {
    auto param = getParameter(id);
    if (param.id.empty() || param.type != ParameterType::BOOL) {
        return defaultValue;
    }
    return param.boolValue;
}

std::string DynamicConfig::getString(const std::string& id, const std::string& defaultValue) const {
    auto param = getParameter(id);
    if (param.id.empty() || (param.type != ParameterType::STRING && param.type != ParameterType::CHOICE)) {
        return defaultValue;
    }
    return param.stringValue;
}

bool DynamicConfig::validateParameterValue(const ConfigParameter& param, const juce::var& value) const {
    switch (param.type) {
        case ParameterType::FLOAT: {
            if (!value.isDouble() && !value.isInt()) return false;
            float floatValue = static_cast<float>(value);
            return floatValue >= param.minValue && floatValue <= param.maxValue;
        }
        case ParameterType::INT: {
            if (!value.isInt()) return false;
            int intValue = static_cast<int>(value);
            return intValue >= static_cast<int>(param.minValue) && intValue <= static_cast<int>(param.maxValue);
        }
        case ParameterType::BOOL:
            return value.isBool();
        case ParameterType::STRING:
            return value.isString();
        case ParameterType::CHOICE: {
            if (!value.isString()) return false;
            std::string stringValue = value.toString().toStdString();
            return std::find(param.choices.begin(), param.choices.end(), stringValue) != param.choices.end();
        }
        default:
            return false;
    }
}

void DynamicConfig::notifyListeners(const std::string& paramId) {
    for (const auto& listener : _listeners) {
        try {
            listener(paramId);
        } catch (...) {
            // Ignora exceções em listeners
        }
    }
}

} // namespace AudioCoPilot