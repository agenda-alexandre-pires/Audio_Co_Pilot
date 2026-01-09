#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <mutex>

class DeviceManager
{
public:
    DeviceManager();
    ~DeviceManager();

    bool initialise(const juce::String& preferredDevice = "");
    
    juce::AudioDeviceManager& getAudioDeviceManager() { return _deviceManager; }
    
    juce::StringArray getAvailableDevices();
    juce::String getCurrentDevice() const;
    bool setDevice(const juce::String& deviceName);
    
    int getCurrentSampleRate() const;
    int getCurrentBufferSize() const;
    int getNumInputChannels() const;

    // Callback para mudanças de device
    std::function<void()> onDeviceChanged;

private:
    juce::AudioDeviceManager _deviceManager;
    
    // Mutex para proteger operações no device manager
    mutable std::mutex _deviceMutex;
    
    // Flag para evitar operações concorrentes
    std::atomic<bool> _changingDevice { false };
    
    void setupDefaultDevice();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceManager)
};

