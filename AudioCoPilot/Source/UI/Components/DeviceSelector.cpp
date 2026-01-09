#include "DeviceSelector.h"
#include "../Colours.h"

DeviceSelector::DeviceSelector()
{
    _label = std::make_unique<juce::Label>("deviceLabel", "INPUT:");
    _label->setFont(juce::Font(juce::FontOptions("Helvetica Neue", 12.0f, juce::Font::bold)));
    _label->setColour(juce::Label::textColourId, Colours::TextSecondary);
    _label->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(*_label);

    _comboBox = std::make_unique<juce::ComboBox>("deviceCombo");
    _comboBox->addListener(this);
    _comboBox->setTextWhenNothingSelected("Select Device...");
    addAndMakeVisible(*_comboBox);
}

void DeviceSelector::resized()
{
    auto bounds = getLocalBounds();
    _label->setBounds(bounds.removeFromLeft(60));
    bounds.removeFromLeft(5);
    _comboBox->setBounds(bounds);
}

void DeviceSelector::setDeviceManager(DeviceManager* manager)
{
    _deviceManager = manager;
    refreshDeviceList();
    
    if (_deviceManager)
    {
        _deviceManager->onDeviceChanged = [this]() { refreshDeviceList(); };
    }
}

void DeviceSelector::refreshDeviceList()
{
    // CORREÇÃO: Atualiza a lista de dispositivos de forma assíncrona
    juce::MessageManager::callAsync([this]() {
        _comboBox->clear(juce::dontSendNotification);
        
        if (_deviceManager == nullptr)
            return;
        
        try {
            auto devices = _deviceManager->getAvailableDevices();
            auto currentDevice = _deviceManager->getCurrentDevice();
            
            int selectedId = 0;
            int id = 1;
            
            for (const auto& device : devices)
            {
                _comboBox->addItem(device, id);
                if (device == currentDevice)
                    selectedId = id;
                ++id;
            }
            
            if (selectedId > 0)
                _comboBox->setSelectedId(selectedId, juce::dontSendNotification);
        }
        catch (const std::exception& e)
        {
            DBG("Error refreshing device list: " << e.what());
        }
        catch (...)
        {
            DBG("Unknown error refreshing device list");
        }
    });
}

void DeviceSelector::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox != _comboBox.get() || _deviceManager == nullptr)
        return;
    
    auto selectedDevice = _comboBox->getText();
    if (selectedDevice.isNotEmpty())
    {
        // CORREÇÃO: Muda o dispositivo de forma assíncrona para evitar deadlock
        juce::MessageManager::callAsync([this, selectedDevice]() {
            try {
                DBG("DeviceSelector: Setting device to " << selectedDevice);
                bool success = _deviceManager->setDevice(selectedDevice);
                
                if (success && onAudioShouldEnable)
                {
                    DBG("DeviceSelector: Device set successfully, calling onAudioShouldEnable");
                    // Chama IMEDIATAMENTE para iniciar áudio
                    if (onAudioShouldEnable)
                        onAudioShouldEnable();
                }
                else if (!success)
                {
                    DBG("DeviceSelector: Failed to set device");
                    // Tenta recarregar a lista em caso de erro
                    refreshDeviceList();
                }
            }
            catch (const std::exception& e)
            {
                DBG("Error setting device: " << e.what());
                refreshDeviceList();
            }
            catch (...)
            {
                DBG("Unknown error setting device");
                refreshDeviceList();
            }
        });
    }
}

