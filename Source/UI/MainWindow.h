#pragma once

#include "../JuceHeader.h"
#include "../Core/DeviceManager.h"
#include "../Core/DeviceStateModel.h"
#include "../Core/AudioEngine.h"
#include "../Menu/MenuBarModel.h"
#include "DeviceSelectorComponent.h"
#include "ChannelMeterComponent.h"
#include "LookAndFeel/IndustrialLookAndFeel.h"
#include "DesignSystem/DesignSystem.h"

namespace AudioCoPilot { 
    class AntiMaskingController; class AntiMaskingView; 
    class RTAController; class RTAView;
    class AIStageHandController; class AIStageHandView;
}

/**
 * MainWindow
 * 
 * Main application window.
 * Integrates device selector, meters, and menu system.
 */
class MainContentComponent : public juce::Component,
                             public juce::ChangeListener
{
public:
    MainContentComponent();
    ~MainContentComponent() override;
    
    void resized() override;
    void paint (juce::Graphics& g) override;
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    
    DeviceSelectorComponent* deviceSelector = nullptr;
    ChannelMeterComponent* inputMeters = nullptr;
    ChannelMeterComponent* outputMeters = nullptr;
    juce::Viewport* inputViewport = nullptr;
    juce::Viewport* outputViewport = nullptr;
    juce::MenuBarComponent* menuBar = nullptr;
    class TransferFunctionView* tfView = nullptr;
    AudioCoPilot::AntiMaskingView* antiMaskingView = nullptr;

    juce::Label footer;
    juce::ImageComponent brandLogo;
    static constexpr int footerHeight = 20;
    static constexpr int brandHeaderHeight = 80;
    
    // Layout constants - horizontal meters need height for all channels
    static constexpr int topBarHeight = 120; // Height for card layout (title + channels label + combo + padding)
    static constexpr int meterAreaHeight = 900; // Initial height, will adjust based on channels
    
private:
    AppLookAndFeel customLF; // Instância do LookAndFeel customizado
};

class MainWindow : public juce::DocumentWindow,
                   public juce::ChangeListener
{
public:
    MainWindow(juce::String name);
    ~MainWindow() override;
    
    void closeButtonPressed() override;
    
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    
private:
    void setupUI();
    
    // Core components
    std::unique_ptr<DeviceStateModel> deviceStateModel;
    std::unique_ptr<DeviceManager> deviceManager;
    std::unique_ptr<AudioEngine> audioEngine;
    
    // UI components
    std::unique_ptr<MainContentComponent> contentComponent;
    std::unique_ptr<DeviceSelectorComponent> deviceSelector;
    std::unique_ptr<ChannelMeterComponent> inputMeters;
    std::unique_ptr<ChannelMeterComponent> outputMeters;
    std::unique_ptr<juce::Viewport> inputViewport;
    std::unique_ptr<juce::Viewport> outputViewport;
    
    // Transfer Function module
    std::unique_ptr<class TFController> tfController;
    std::unique_ptr<class TransferFunctionView> tfView;

    // Anti-Masking module
    std::unique_ptr<AudioCoPilot::AntiMaskingController> antiMaskingController;
    std::unique_ptr<AudioCoPilot::AntiMaskingView> antiMaskingView;

    std::unique_ptr<AudioCoPilot::RTAController> rtaController;
    std::unique_ptr<AudioCoPilot::RTAView> rtaView;

    std::unique_ptr<AudioCoPilot::AIStageHandController> aiStageHandController;
    std::unique_ptr<AudioCoPilot::AIStageHandView> aiStageHandView;
    
    // Menu
    std::unique_ptr<MenuBarModel> menuBarModel;
    std::unique_ptr<juce::MenuBarComponent> menuBar;
    
    // Look and Feel
    std::unique_ptr<AudioCoPilot::IndustrialLookAndFeel> lookAndFeel;
    
    // Module management
    void showTransferFunction();
    void hideTransferFunction();

    
    void showAntiMasking();
    void hideAntiMasking();

    void showRTA();
    void hideRTA();

    void showAIStageHand();
    void hideAIStageHand();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
