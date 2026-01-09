#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <vector>
#include <string>
#include <functional>
#include <mutex>

namespace AudioCoPilot {

// Tipos de parâmetros suportados
enum class ParameterType {
    FLOAT,
    INT,
    BOOL,
    STRING,
    CHOICE
};

// Estrutura para um parâmetro configurável
struct ConfigParameter {
    std::string id;
    std::string name;
    std::string description;
    ParameterType type;
    
    // Valores (dependendo do tipo)
    union {
        float floatValue;
        int intValue;
        bool boolValue;
    };
    std::string stringValue;
    std::vector<std::string> choices;
    
    // Range para valores numéricos
    float minValue = 0.0f;
    float maxValue = 100.0f;
    float defaultValue = 0.0f;
    
    // Callback para mudanças
    std::function<void(const ConfigParameter&)> onChangeCallback;
    
    // Estado
    bool isReadOnly = false;
    bool requiresRestart = false;
    
    ConfigParameter(const std::string& id, const std::string& name, ParameterType type)
        : id(id), name(name), type(type) {}
};

// Sistema de configuração dinâmica
class DynamicConfig {
public:
    static DynamicConfig& getInstance();
    
    // Gerenciamento de parâmetros
    void registerParameter(const ConfigParameter& param);
    bool updateParameter(const std::string& id, const juce::var& value);
    ConfigParameter getParameter(const std::string& id) const;
    std::vector<ConfigParameter> getAllParameters() const;
    
    // Grupos de parâmetros
    void createGroup(const std::string& groupName, const std::vector<std::string>& paramIds);
    std::vector<std::string> getParametersInGroup(const std::string& groupName) const;
    
    // Persistência
    void loadFromFile(const juce::File& file);
    void saveToFile(const juce::File& file) const;
    void loadDefaults();
    
    // Notificações
    void addListener(std::function<void(const std::string& paramId)> listener);
    void removeListener(std::function<void(const std::string& paramId)> listener);
    
    // Acesso rápido a parâmetros comuns
    float getFloat(const std::string& id, float defaultValue = 0.0f) const;
    int getInt(const std::string& id, int defaultValue = 0) const;
    bool getBool(const std::string& id, bool defaultValue = false) const;
    std::string getString(const std::string& id, const std::string& defaultValue = "") const;
    
    // Validação
    bool validateParameterValue(const ConfigParameter& param, const juce::var& value) const;
    
private:
    DynamicConfig();
    ~DynamicConfig() = default;
    
    DynamicConfig(const DynamicConfig&) = delete;
    DynamicConfig& operator=(const DynamicConfig&) = delete;
    
    void initializeDefaultParameters();
    void notifyListeners(const std::string& paramId);
    
    mutable std::mutex _mutex;
    std::vector<ConfigParameter> _parameters;
    std::map<std::string, std::vector<std::string>> _groups;
    std::vector<std::function<void(const std::string&)>> _listeners;
    
    // IDs de parâmetros padrão
    static constexpr const char* PARAM_THRESHOLD_PNPR = "threshold_pnpr";
    static constexpr const char* PARAM_THRESHOLD_PHPR = "threshold_phpr";
    static constexpr const char* PARAM_MIN_PERSISTENCE = "min_persistence";
    static constexpr const char* PARAM_MIN_SLOPE = "min_slope";
    static constexpr const char* PARAM_LOOP_GAIN_THRESHOLD = "loop_gain_threshold";
    static constexpr const char* PARAM_RMS_ALPHA = "rms_alpha";
    static constexpr const char* PARAM_ENABLE_ADAPTIVE = "enable_adaptive";
    static constexpr const char* PARAM_ENABLE_LEARNING = "enable_learning";
    static constexpr const char* PARAM_LEARN_FRAMES = "learn_frames";
    static constexpr const char* PARAM_ENABLE_LOGGING = "enable_logging";
    static constexpr const char* PARAM_LOG_LEVEL = "log_level";
    static constexpr const char* PARAM_ENABLE_PERF_MONITOR = "enable_perf_monitor";
    static constexpr const char* PARAM_PERF_SAMPLING_INTERVAL = "perf_sampling_interval";
    static constexpr const char* PARAM_ENABLE_AUTO_EXPORT = "enable_auto_export";
    static constexpr const char* PARAM_EXPORT_INTERVAL = "export_interval";
    static constexpr const char* PARAM_EXPORT_FORMAT = "export_format";
};

// Macros para fácil acesso
#define CONFIG_GET_FLOAT(id, defaultValue) \
    AudioCoPilot::DynamicConfig::getInstance().getFloat(id, defaultValue)

#define CONFIG_GET_INT(id, defaultValue) \
    AudioCoPilot::DynamicConfig::getInstance().getInt(id, defaultValue)

#define CONFIG_GET_BOOL(id, defaultValue) \
    AudioCoPilot::DynamicConfig::getInstance().getBool(id, defaultValue)

#define CONFIG_GET_STRING(id, defaultValue) \
    AudioCoPilot::DynamicConfig::getInstance().getString(id, defaultValue)

} // namespace AudioCoPilot