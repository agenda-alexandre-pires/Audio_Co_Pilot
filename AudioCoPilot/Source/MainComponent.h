#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "Core/AudioEngine.h"
#include "Core/DeviceManager.h"
#include "UI/Components/HeaderBar.h"
#include "UI/Components/MeterBridge.h"
#include "UI/Components/StatusBar.h"
#include "Modules/TransferFunction/TransferFunctionComponent.h"
#include "Modules/FeedbackPrediction/FeedbackPredictionComponent.h"
#include "Modules/AntiMasking/AntiMaskingComponent.h"

class MainComponent : public juce::Component,
                      public juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    // Gerenciadores
    std::unique_ptr<DeviceManager> _deviceManager;
    std::unique_ptr<AudioEngine> _audioEngine;

    // Componentes UI
    std::unique_ptr<HeaderBar> _headerBar;
    std::unique_ptr<MeterBridge> _meterBridge;
    std::unique_ptr<TransferFunctionComponent> _transferFunctionComponent;
    std::unique_ptr<AudioCoPilot::FeedbackPredictionComponent> _feedbackPredictionComponent;
    std::unique_ptr<AudioCoPilot::AntiMaskingComponent> _antiMaskingComponent;
    std::unique_ptr<StatusBar> _statusBar;

    // Estado
    bool _audioRunning = false;
    enum class ViewMode { Meters, TransferFunction, FeedbackPrediction, AntiMasking };
    ViewMode _currentViewMode = ViewMode::Meters;
    
    // Proteção contra reentrância
    std::atomic<bool> _switchingView { false };
    
    // Modo de segurança - desabilita inicialização automática de áudio
    bool _safeMode = true;
    
    // Layout
    juce::Rectangle<int> _centralBounds;

    void setupAudio();
    void updateMeters();
    void switchViewMode(ViewMode mode);
    void checkDeviceHealth();
    void initializeAudioOnce(); // Inicializa áudio UMA ÚNICA VEZ
    
    juce::String viewModeToString(ViewMode mode);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
