#pragma once

#include "../JuceHeader.h"
#include "../Core/DeviceManager.h"
#include "../Localization/LocalizedStrings.h"

/**
 * DeviceSelectorComponent
 * 
 * macOS-style device selector with separate Input and Output dropdowns.
 * Updates device list and handles device switching safely.
 */
class DeviceSelectorComponent : public juce::Component,
                                 public juce::ComboBox::Listener,
                                 public juce::ChangeListener
{
public:
    DeviceSelectorComponent(DeviceManager& deviceManager);
    ~DeviceSelectorComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    
    void refreshDeviceList();
    
private:
    void populateDeviceLists();
    void updateSelectedDevices();
    
    DeviceManager& deviceManager;
    
    juce::ComboBox inputDeviceComboBox;
    juce::ComboBox outputDeviceComboBox;
    juce::Label inputDeviceLabel;
    juce::Label outputDeviceLabel;
    
    juce::String currentInputDeviceName;
    juce::String currentOutputDeviceName;
    
    bool isUpdating{false};
    
    // Áreas dos cards para posicionamento
    juce::Rectangle<int> inputCardArea;
    juce::Rectangle<int> outputCardArea;
    
    // LookAndFeel customizado
    AppLookAndFeel customLF;
};
