#pragma once

#include "../../JuceHeader.h"
#include "AntiMaskingController.h"
#include "AntiMaskingHeaderComponent.h"
#include "AntiMaskingFrequencyGraphComponent.h"
#include "MaskingSourceCardComponent.h"
#include "AISuggestionsEngine.h"
#include <array>

namespace AudioCoPilot
{

/**
 * AntiMaskingViewModern
 * 
 * Versão refatorada do Anti-Masking seguindo o design moderno do Stitch
 * Layout: Header → Breadcrumbs → Actions → Main Graph → Masking Sources Cards → Footer
 */
class AntiMaskingViewModern : public juce::Component,
                              public juce::ComboBox::Listener,
                              public juce::Button::Listener,
                              public juce::ChangeListener,
                              public juce::Timer
{
public:
    AntiMaskingViewModern(AntiMaskingController& c);
    ~AntiMaskingViewModern() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked(juce::Button* button) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void timerCallback() override;

private:
    void rebuildChannelLists();
    void updateFrequencyGraph();
    void updateMaskingSourceCards();
    float calculateOverallMaskingSeverity() const;

    AntiMaskingController& controller;

    // Header
    AntiMaskingHeaderComponent headerComponent;

    // Breadcrumbs
    juce::Label breadcrumbsLabel;

    // Action buttons
    juce::TextButton autoCorrectButton;
    juce::TextButton freezeSceneButton;

    // Target Channel info
    juce::Label targetChannelLabel;
    juce::ComboBox targetCombo;
    juce::Label peakLabel;

    // Masking Severity card
    juce::Label maskingSeverityLabel;
    juce::Label maskingSeverityValue;

    // Main frequency graph
    AntiMaskingFrequencyGraphComponent frequencyGraph;

    // Masking Sources cards
    std::array<MaskingSourceCardComponent, 3> sourceCards;

    // Footer
    juce::Label spectralEngineLabel;
    juce::Label fftSizeLabel;
    juce::Slider smoothingSlider;
    juce::Label utcTimeLabel;

    // Channel selectors (hidden, for controller)
    std::array<juce::ToggleButton, 3> maskerEnable;
    std::array<juce::ComboBox, 3> maskerCombo;

    // Data
    float currentPeakDb{-100.0f};
    float currentPeakFreq{0.0f};
    float overallMaskingSeverity{0.0f};
    
    static constexpr int headerHeight = 60;
    static constexpr int breadcrumbsHeight = 30;
    static constexpr int actionButtonsHeight = 50;
    static constexpr int targetInfoHeight = 40;
    static constexpr int severityCardHeight = 80;
    static constexpr int footerHeight = 40;
};

}
