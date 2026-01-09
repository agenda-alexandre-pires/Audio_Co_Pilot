#include "DeviceManager.h"

DeviceManager::DeviceManager()
{
}

DeviceManager::~DeviceManager()
{
    _deviceManager.closeAudioDevice();
}

bool DeviceManager::initialise(const juce::String& preferredDevice)
{
    std::lock_guard<std::mutex> lock(_deviceMutex);
    
    std::cout << "[DeviceManager] initialise() called" << std::endl;
    std::cout << "[DeviceManager] Preferred device: " << preferredDevice.toStdString() << std::endl;
    
    // Verifica se já está inicializado
    if (auto* currentDevice = _deviceManager.getCurrentAudioDevice())
    {
        std::cout << "[DeviceManager] Device already initialized: " << currentDevice->getName().toStdString() << std::endl;
        return true;
    }
    
    // Configuração: 2 inputs, 0 outputs (só análise, não reprodução)
    std::cout << "[DeviceManager] Calling _deviceManager.initialise()..." << std::endl;
    auto result = _deviceManager.initialise(
        64,     // numInputChannels (máximo, vai usar o que tiver)
        0,      // numOutputChannels (não precisamos de saída)
        nullptr, // savedState
        true,   // selectDefaultDeviceOnFailure
        preferredDevice,
        nullptr  // preferredSetupOptions
    );
    
    if (result.isNotEmpty())
    {
        std::cerr << "[DeviceManager] Audio init ERROR: " << result.toStdString() << std::endl;
        DBG("Audio init error: " << result);
        return false;
    }
    
    // Verifica se dispositivo foi realmente aberto
    if (auto* device = _deviceManager.getCurrentAudioDevice())
    {
        std::cout << "[DeviceManager] Device successfully opened: " << device->getName().toStdString() << std::endl;
        std::cout << "[DeviceManager] Sample rate: " << device->getCurrentSampleRate() << " Hz" << std::endl;
        std::cout << "[DeviceManager] Buffer size: " << device->getCurrentBufferSizeSamples() << " samples" << std::endl;
        std::cout << "[DeviceManager] Input channels: " << device->getActiveInputChannels().countNumberOfSetBits() << std::endl;
        std::cout << "[DeviceManager] Output channels: " << device->getActiveOutputChannels().countNumberOfSetBits() << std::endl;
        
        if (device->getActiveInputChannels().countNumberOfSetBits() == 0)
        {
            std::cerr << "[DeviceManager] WARNING: Device opened but has NO input channels!" << std::endl;
            std::cerr << "[DeviceManager] This may indicate:" << std::endl;
            std::cerr << "[DeviceManager]   1. macOS microphone permission not granted" << std::endl;
            std::cerr << "[DeviceManager]   2. Device has no input capability" << std::endl;
            std::cerr << "[DeviceManager]   3. Device not properly configured" << std::endl;
        }
    }
    else
    {
        std::cerr << "[DeviceManager] ERROR: initialise() returned success but no device is open!" << std::endl;
        return false;
    }
    
    std::cout << "[DeviceManager] initialise() completed successfully" << std::endl;
    return true;
}

juce::StringArray DeviceManager::getAvailableDevices()
{
    std::lock_guard<std::mutex> lock(_deviceMutex);
    
    juce::StringArray devices;
    
    auto& deviceTypes = _deviceManager.getAvailableDeviceTypes();
    
    for (auto* type : deviceTypes)
    {
        type->scanForDevices();
        auto deviceNames = type->getDeviceNames(true); // true = input devices
        
        for (const auto& name : deviceNames)
            devices.add(name);
    }
    
    return devices;
}

juce::String DeviceManager::getCurrentDevice() const
{
    std::lock_guard<std::mutex> lock(_deviceMutex);
    
    if (auto* device = _deviceManager.getCurrentAudioDevice())
        return device->getName();
    
    return "No Device";
}

bool DeviceManager::setDevice(const juce::String& deviceName)
{
    std::cout << "[DeviceManager] setDevice() called: " << deviceName.toStdString() << std::endl;
    
    // Evita múltiplas mudanças concorrentes
    bool expected = false;
    if (!_changingDevice.compare_exchange_strong(expected, true)) {
        std::cout << "[DeviceManager] Device change already in progress, skipping" << std::endl;
        DBG("Device change already in progress, skipping");
        return false;
    }
    
    // Auto-reset da flag no final
    auto cleanup = [this]() { _changingDevice.store(false); };
    
    try {
        std::lock_guard<std::mutex> lock(_deviceMutex);
        
        // CORREÇÃO CRÍTICA: Se dispositivo não está inicializado, inicializa primeiro
        if (!_deviceManager.getCurrentAudioDevice())
        {
            std::cout << "[DeviceManager] Device not initialized, initializing..." << std::endl;
            
            // Já temos lock, então chama diretamente sem lock adicional
            auto result = _deviceManager.initialise(
                64,     // numInputChannels
                0,      // numOutputChannels
                nullptr, // savedState
                true,   // selectDefaultDeviceOnFailure
                deviceName,
                nullptr  // preferredSetupOptions
            );
            
            if (result.isNotEmpty())
            {
                std::cerr << "[DeviceManager] Failed to initialize device: " << result.toStdString() << std::endl;
                cleanup();
                return false;
            }
            
            std::cout << "[DeviceManager] Device initialized successfully" << std::endl;
        }
        else
        {
            // Dispositivo já está inicializado, apenas muda
            std::cout << "[DeviceManager] Device already initialized, changing to: " << deviceName.toStdString() << std::endl;
            
            auto setup = _deviceManager.getAudioDeviceSetup();
            setup.inputDeviceName = deviceName;
            
            auto result = _deviceManager.setAudioDeviceSetup(setup, true);
            
            if (result.isNotEmpty())
            {
                std::cerr << "[DeviceManager] Failed to set device: " << result.toStdString() << std::endl;
                DBG("Failed to set device: " << result);
                cleanup();
                return false;
            }
            
            std::cout << "[DeviceManager] Device changed successfully" << std::endl;
        }
        
        // Verifica se dispositivo está realmente aberto
        if (auto* device = _deviceManager.getCurrentAudioDevice())
        {
            std::cout << "[DeviceManager] Current device: " << device->getName().toStdString() << std::endl;
            std::cout << "[DeviceManager] Input channels active: " << device->getActiveInputChannels().countNumberOfSetBits() << std::endl;
        }
        else
        {
            std::cerr << "[DeviceManager] ERROR: Device set but not open!" << std::endl;
            cleanup();
            return false;
        }
        
        if (onDeviceChanged)
        {
            // Chama o callback de forma assíncrona para evitar deadlock
            juce::MessageManager::callAsync([this]() {
                if (onDeviceChanged)
                    onDeviceChanged();
            });
        }
        
        cleanup();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DeviceManager] EXCEPTION in setDevice: " << e.what() << std::endl;
        cleanup();
        return false;
    }
    catch (...)
    {
        std::cerr << "[DeviceManager] UNKNOWN EXCEPTION in setDevice" << std::endl;
        cleanup();
        return false;
    }
}

int DeviceManager::getCurrentSampleRate() const
{
    if (auto* device = _deviceManager.getCurrentAudioDevice())
        return static_cast<int>(device->getCurrentSampleRate());
    
    return 0;
}

int DeviceManager::getCurrentBufferSize() const
{
    if (auto* device = _deviceManager.getCurrentAudioDevice())
        return device->getCurrentBufferSizeSamples();
    
    return 0;
}

int DeviceManager::getNumInputChannels() const
{
    if (auto* device = _deviceManager.getCurrentAudioDevice())
        return device->getActiveInputChannels().countNumberOfSetBits();
    
    return 0;
}

