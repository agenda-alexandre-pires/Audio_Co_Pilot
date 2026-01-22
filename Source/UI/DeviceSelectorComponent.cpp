#include "DeviceSelectorComponent.h"
#include "DesignSystem/DesignSystem.h"

DeviceSelectorComponent::DeviceSelectorComponent(DeviceManager& deviceManager)
    : deviceManager(deviceManager)
{
    using namespace AudioCoPilot::DesignSystem;
    
    auto& strings = LocalizedStrings::getInstance();
    
    // Input device selector
    inputDeviceLabel.setText(strings.getInputSelector() + ":", juce::dontSendNotification);
    inputDeviceLabel.setFont(Typography::labelLarge());
    inputDeviceLabel.setColour(juce::Label::textColourId, Colours::getColour(Colours::Text::Primary));
    addAndMakeVisible(inputDeviceLabel);
    
    inputDeviceComboBox.addListener(this);
    addAndMakeVisible(inputDeviceComboBox);
    
    // Output device selector
    outputDeviceLabel.setText(strings.getOutputSelector() + ":", juce::dontSendNotification);
    outputDeviceLabel.setFont(Typography::labelLarge());
    outputDeviceLabel.setColour(juce::Label::textColourId, Colours::getColour(Colours::Text::Primary));
    addAndMakeVisible(outputDeviceLabel);
    
    outputDeviceComboBox.addListener(this);
    addAndMakeVisible(outputDeviceComboBox);
    
    // Listen to device manager changes
    deviceManager.getAudioDeviceManager().addChangeListener(this);
    
    populateDeviceLists();
    updateSelectedDevices();
    
    strings.addChangeListener(this);
}

DeviceSelectorComponent::~DeviceSelectorComponent()
{
    inputDeviceComboBox.removeListener(this);
    outputDeviceComboBox.removeListener(this);
    deviceManager.getAudioDeviceManager().removeChangeListener(this);
    LocalizedStrings::getInstance().removeChangeListener(this);
}

void DeviceSelectorComponent::paint(juce::Graphics& g)
{
    // Background handled by parent
}

void DeviceSelectorComponent::resized()
{
    auto bounds = getLocalBounds();
    const int comboHeight = 28;
    const int labelWidth = 120;  // Increased to accommodate "Input Selector" and "Output Selector"
    const int spacing = 10;
    
    // Split into two halves: Input (left 50%) and Output (right 50%)
    auto inputArea = bounds.removeFromLeft(bounds.getWidth() / 2);
    auto outputArea = bounds;
    
    // Input: Label + ComboBox on same line
    auto inputRow = inputArea.removeFromTop(comboHeight);
    inputDeviceLabel.setBounds(inputRow.removeFromLeft(labelWidth).reduced(5, 0));
    inputDeviceComboBox.setBounds(inputRow.reduced(5, 0));
    
    // Output: Label + ComboBox on same line
    auto outputRow = outputArea.removeFromTop(comboHeight);
    outputDeviceLabel.setBounds(outputRow.removeFromLeft(labelWidth).reduced(5, 0));
    outputDeviceComboBox.setBounds(outputRow.reduced(5, 0));
}

void DeviceSelectorComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (isUpdating)
        return;
    
    if (comboBoxThatHasChanged == &inputDeviceComboBox)
    {
        int selectedId = inputDeviceComboBox.getSelectedId();
        if (selectedId > 0)
        {
            juce::String selectedDevice = inputDeviceComboBox.getItemText(inputDeviceComboBox.getSelectedItemIndex());
            deviceManager.selectInputDevice(selectedDevice);
        }
        else
        {
            // "None" selected
            deviceManager.selectInputDevice(juce::String());
        }
    }
    else if (comboBoxThatHasChanged == &outputDeviceComboBox)
    {
        int selectedId = outputDeviceComboBox.getSelectedId();
        if (selectedId > 0)
        {
            juce::String selectedDevice = outputDeviceComboBox.getItemText(outputDeviceComboBox.getSelectedItemIndex());
            deviceManager.selectOutputDevice(selectedDevice);
        }
        else
        {
            // "None" selected
            deviceManager.selectOutputDevice(juce::String());
        }
    }
}

void DeviceSelectorComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &LocalizedStrings::getInstance())
    {
        repaint();
    }
    else if (source == &deviceManager.getAudioDeviceManager())
    {
        // Device changed, update selection and repaint to refresh channel counts
        updateSelectedDevices();
        repaint();
    }
}

void DeviceSelectorComponent::refreshDeviceList()
{
    populateDeviceLists();
}

void DeviceSelectorComponent::populateDeviceLists()
{
    isUpdating = true;
    
    // Clear combo boxes
    inputDeviceComboBox.clear();
    outputDeviceComboBox.clear();
    
    // Get available devices
    auto inputDevices = deviceManager.getAvailableInputDevices();
    auto outputDevices = deviceManager.getAvailableOutputDevices();
    
    // Populate input combo box
    auto& strings = LocalizedStrings::getInstance();
    inputDeviceComboBox.addItem(strings.getNoDeviceSelected(), 1);
    
    int itemId = 2;
    for (const auto& device : inputDevices)
    {
        inputDeviceComboBox.addItem(device, itemId++);
    }
    
    // Populate output combo box
    outputDeviceComboBox.addItem(strings.getNoDeviceSelected(), 1);
    
    itemId = 2;
    for (const auto& device : outputDevices)
    {
        outputDeviceComboBox.addItem(device, itemId++);
    }
    
    updateSelectedDevices();
    
    isUpdating = false;
}

void DeviceSelectorComponent::updateSelectedDevices()
{
    if (isUpdating)
        return;
    
    isUpdating = true;
    
    juce::String currentInput = deviceManager.getCurrentInputDeviceName();
    juce::String currentOutput = deviceManager.getCurrentOutputDeviceName();
    
    // Update input selection
    if (currentInput != currentInputDeviceName)
    {
        currentInputDeviceName = currentInput;
        
        if (currentInput.isNotEmpty())
        {
            auto inputDevices = deviceManager.getAvailableInputDevices();
            int index = inputDevices.indexOf(currentInput);
            if (index >= 0)
            {
                inputDeviceComboBox.setSelectedId(index + 2, juce::dontSendNotification);
            }
            else
            {
                inputDeviceComboBox.setSelectedId(1, juce::dontSendNotification); // "No device"
            }
        }
        else
        {
            inputDeviceComboBox.setSelectedId(1, juce::dontSendNotification); // "No device"
        }
    }
    
    // Update output selection
    if (currentOutput != currentOutputDeviceName)
    {
        currentOutputDeviceName = currentOutput;
        
        if (currentOutput.isNotEmpty())
        {
            auto outputDevices = deviceManager.getAvailableOutputDevices();
            int index = outputDevices.indexOf(currentOutput);
            if (index >= 0)
            {
                outputDeviceComboBox.setSelectedId(index + 2, juce::dontSendNotification);
            }
            else
            {
                outputDeviceComboBox.setSelectedId(1, juce::dontSendNotification); // "No device"
            }
        }
        else
        {
            outputDeviceComboBox.setSelectedId(1, juce::dontSendNotification); // "No device"
        }
    }
    
    isUpdating = false;
}
