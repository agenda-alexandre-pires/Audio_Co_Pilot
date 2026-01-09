#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Core/DeviceManager.h"

class DeviceSelector : public juce::Component,
                       public juce::ComboBox::Listener
{
public:
    DeviceSelector();
    ~DeviceSelector() override = default;

    void resized() override;
    void setDeviceManager(DeviceManager* manager);
    
    // Callback para quando áudio deve ser habilitado
    std::function<void()> onAudioShouldEnable;

    void comboBoxChanged(juce::ComboBox* comboBox) override;

private:
    std::unique_ptr<juce::Label> _label;
    std::unique_ptr<juce::ComboBox> _comboBox;
    DeviceManager* _deviceManager = nullptr;

    void refreshDeviceList();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceSelector)
};

