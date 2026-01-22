#include "AntiMaskingViewModern.h"
#include "../../Localization/LocalizedStrings.h"
#include <ctime>

namespace AudioCoPilot
{

AntiMaskingViewModern::AntiMaskingViewModern(AntiMaskingController& c)
    : controller(c)
{
    auto& strings = LocalizedStrings::getInstance();

    // Header
    addAndMakeVisible(headerComponent);
    headerComponent.setCPUUsage(14.2f);
    headerComponent.setSampleRate(48000, 24);

    // Breadcrumbs
    breadcrumbsLabel.setText("Projects / Neon Nights Remix / Anti-Masking Analysis", 
                            juce::dontSendNotification);
    breadcrumbsLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
    breadcrumbsLabel.setFont(juce::Font(11.0f));
    addAndMakeVisible(breadcrumbsLabel);

    // Action buttons
    autoCorrectButton.setButtonText("AUTO-CORRECT");
    autoCorrectButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3b82f6)); // Blue
    autoCorrectButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    autoCorrectButton.addListener(this);
    addAndMakeVisible(autoCorrectButton);

    freezeSceneButton.setButtonText("FREEZE SCENE");
    freezeSceneButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1a1a));
    freezeSceneButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    freezeSceneButton.addListener(this);
    addAndMakeVisible(freezeSceneButton);

    // Target Channel info (combined label like design: "Target Channel: **Vocal Lead (Main)**")
    targetChannelLabel.setText("Target Channel: **Ch 1**", juce::dontSendNotification);
    targetChannelLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    targetChannelLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(targetChannelLabel);

    targetCombo.addListener(this);
    targetCombo.setVisible(false); // Hidden, we show selection in label

    peakLabel.setText("PEAK: -12.4 dBFS @ 842 Hz", juce::dontSendNotification);
    peakLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    peakLabel.setFont(juce::Font(11.0f));
    addAndMakeVisible(peakLabel);

    // Masking Severity card
    maskingSeverityLabel.setText("MASKING SEVERITY", juce::dontSendNotification);
    maskingSeverityLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    maskingSeverityLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(maskingSeverityLabel);

    maskingSeverityValue.setText("34.8%", juce::dontSendNotification);
    maskingSeverityValue.setColour(juce::Label::textColourId, juce::Colour(0xffff6b35)); // Orange
    maskingSeverityValue.setFont(juce::Font(24.0f, juce::Font::bold));
    addAndMakeVisible(maskingSeverityValue);

    // Frequency graph
    addAndMakeVisible(frequencyGraph);

    // Masking Sources cards
    sourceCards[0].setColour(juce::Colours::red);
    sourceCards[1].setColour(juce::Colour(0xffff6b35)); // Orange
    sourceCards[2].setColour(juce::Colour(0xff4ade80)); // Green
    for (auto& card : sourceCards)
        addAndMakeVisible(card);

    // Footer
    spectralEngineLabel.setText("SPECTRAL ENGINE: ACTIVE", juce::dontSendNotification);
    spectralEngineLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4ade80));
    spectralEngineLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(spectralEngineLabel);

    fftSizeLabel.setText("FFT SIZE: 4096", juce::dontSendNotification);
    fftSizeLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
    fftSizeLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(fftSizeLabel);

    smoothingSlider.setRange(0.0, 1.0, 0.01);
    smoothingSlider.setValue(0.5);
    smoothingSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(smoothingSlider);

    utcTimeLabel.setText("UTC 14:23:05", juce::dontSendNotification);
    utcTimeLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.6f));
    utcTimeLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(utcTimeLabel);

    // Hidden controls for controller
    for (int i = 0; i < 3; ++i)
    {
        maskerEnable[(size_t)i].addListener(this);
        maskerCombo[(size_t)i].addListener(this);
        addChildComponent(maskerEnable[(size_t)i], false);
        addChildComponent(maskerCombo[(size_t)i], false);
    }

    controller.addChangeListener(this);
    rebuildChannelLists();
    strings.addChangeListener(this);
}

AntiMaskingViewModern::~AntiMaskingViewModern()
{
    stopTimer();
    controller.removeChangeListener(this);
    LocalizedStrings::getInstance().removeChangeListener(this);
}

void AntiMaskingViewModern::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0a0a0a)); // Dark background
}

void AntiMaskingViewModern::resized()
{
    auto bounds = getLocalBounds();

    // Header
    headerComponent.setBounds(bounds.removeFromTop(headerHeight));

    // Breadcrumbs and actions
    auto topBar = bounds.removeFromTop(breadcrumbsHeight + actionButtonsHeight);
    auto breadcrumbsArea = topBar.removeFromTop(breadcrumbsHeight);
    breadcrumbsLabel.setBounds(breadcrumbsArea.reduced(20, 5));

    auto actionsArea = topBar.removeFromRight(300).reduced(10);
    autoCorrectButton.setBounds(actionsArea.removeFromLeft(140).reduced(5, 0));
    freezeSceneButton.setBounds(actionsArea.reduced(5, 0));

    // Target info and severity
    auto infoBar = bounds.removeFromTop(targetInfoHeight + severityCardHeight);
    
    auto targetArea = infoBar.removeFromTop(targetInfoHeight).reduced(20, 5);
    targetChannelLabel.setBounds(targetArea.removeFromLeft(300));
    peakLabel.setBounds(targetArea.reduced(10, 0));

    auto severityArea = infoBar.removeFromRight(200).reduced(10);
    maskingSeverityLabel.setBounds(severityArea.removeFromTop(20));
    maskingSeverityValue.setBounds(severityArea);

    // Main frequency graph (takes most space)
    auto graphHeight = (int)(bounds.getHeight() * 0.55f);
    frequencyGraph.setBounds(bounds.removeFromTop(graphHeight).reduced(20, 10));

    // Masking Sources cards (horizontal layout)
    auto cardsArea = bounds.removeFromTop(180).reduced(20, 10);
    const int cardWidth = cardsArea.getWidth() / 3;
    for (size_t i = 0; i < sourceCards.size(); ++i)
    {
        sourceCards[i].setBounds(cardsArea.removeFromLeft(cardWidth).reduced(5, 0));
    }

    // Footer
    auto footerArea = bounds.removeFromBottom(footerHeight);
    spectralEngineLabel.setBounds(footerArea.removeFromLeft(180).reduced(10, 0));
    fftSizeLabel.setBounds(footerArea.removeFromLeft(120).reduced(10, 0));
    smoothingSlider.setBounds(footerArea.removeFromLeft(200).reduced(10, 0));
    utcTimeLabel.setBounds(footerArea.reduced(10, 0));
}

void AntiMaskingViewModern::visibilityChanged()
{
    if (isVisible())
    {
        if (!isTimerRunning())
            startTimerHz(20);
    }
    else
    {
        if (isTimerRunning())
            stopTimer();
    }
}

void AntiMaskingViewModern::timerCallback()
{
    if (!isVisible())
    {
        stopTimer();
        return;
    }

    const auto& result = controller.getAveragedResult();
    const auto spectra = controller.getLatestSpectraDb();

    // Update overall masking severity
    overallMaskingSeverity = calculateOverallMaskingSeverity();
    maskingSeverityValue.setText(juce::String(overallMaskingSeverity, 1) + "%", 
                                juce::dontSendNotification);

    // Update color based on severity
    if (overallMaskingSeverity >= 30.0f)
        maskingSeverityValue.setColour(juce::Label::textColourId, juce::Colour(0xffff6b35)); // Orange
    else if (overallMaskingSeverity >= 15.0f)
        maskingSeverityValue.setColour(juce::Label::textColourId, juce::Colour(0xff4ade80)); // Green
    else
        maskingSeverityValue.setColour(juce::Label::textColourId, juce::Colour(0xff60a5fa)); // Light blue

    // Update frequency graph
    updateFrequencyGraph();

    // Update masking source cards
    updateMaskingSourceCards();

    // Update UTC time
    auto now = std::time(nullptr);
    auto* timeInfo = std::localtime(&now);
    juce::String timeStr = juce::String::formatted("UTC %02d:%02d:%02d",
                                                   timeInfo->tm_hour,
                                                   timeInfo->tm_min,
                                                   timeInfo->tm_sec);
    utcTimeLabel.setText(timeStr, juce::dontSendNotification);

    repaint();
}

void AntiMaskingViewModern::updateFrequencyGraph()
{
    const auto& result = controller.getAveragedResult();

    // Generate anti-masking curve from result
    std::vector<float> freqs;
    std::vector<float> gainsDb;

    // Convert Bark bands to frequency curve
    const float barkToFreq[] = {
        20.0f, 100.0f, 200.0f, 300.0f, 400.0f, 510.0f, 630.0f, 770.0f, 920.0f, 1080.0f,
        1270.0f, 1480.0f, 1720.0f, 2000.0f, 2320.0f, 2700.0f, 3150.0f, 3700.0f,
        4400.0f, 5300.0f, 6400.0f, 7700.0f, 9500.0f, 12000.0f
    };

    for (int i = 0; i < 24; ++i)
    {
        freqs.push_back(barkToFreq[i]);
        // Calculate gain needed to overcome masking
        float smr = result.bands[(size_t)i].smrDb;
        float gain = juce::jmax(0.0f, -smr * 0.5f); // Simple heuristic
        gainsDb.push_back(gain);
    }

    frequencyGraph.setAntiMaskingCurve(freqs, gainsDb);

    // Generate masking zones
    std::vector<AntiMaskingFrequencyGraphComponent::MaskingZone> zones;
    for (int i = 0; i < 24; ++i)
    {
        if (result.bands[(size_t)i].audibility01 < 0.7f) // Masked
        {
            AntiMaskingFrequencyGraphComponent::MaskingZone zone;
            zone.centerFreq = barkToFreq[i];
            zone.bandwidth = 0.33f; // 1/3 octave
            zone.severity = 1.0f - result.bands[(size_t)i].audibility01;
            zones.push_back(zone);
        }
    }
    frequencyGraph.setMaskingZones(zones);
}

void AntiMaskingViewModern::updateMaskingSourceCards()
{
    const auto& result = controller.getAveragedResult();
    const auto spectra = controller.getLatestSpectraDb();

    // Get masker names (simplified - use channel numbers)
    juce::String maskerNames[] = {"KICK DRUM", "BASS GUITAR", "SYNTH LEAD"};

    for (int i = 0; i < 3; ++i)
    {
        if (controller.isMaskerEnabled(i))
        {
            // Calculate masking impact for this masker
            float totalImpact = 0.0f;
            for (int b = 0; b < 24; ++b)
            {
                if (result.bands[(size_t)b].dominantMasker == i)
                {
                    totalImpact += (1.0f - result.bands[(size_t)b].audibility01);
                }
            }
            float impactPercent = (totalImpact / 24.0f) * 100.0f;

            // Calculate average level
            float avgLevel = 0.0f;
            for (int b = 0; b < 24; ++b)
                avgLevel += spectra[(size_t)(i + 1)][(size_t)b];
            avgLevel /= 24.0f;

            sourceCards[(size_t)i].setSourceInfo(
                i,
                maskerNames[i],
                avgLevel,
                impactPercent,
                spectra[(size_t)(i + 1)]
            );
        }
    }
}

float AntiMaskingViewModern::calculateOverallMaskingSeverity() const
{
    const auto& result = controller.getAveragedResult();
    float totalMasked = 0.0f;
    
    for (const auto& band : result.bands)
    {
        totalMasked += (1.0f - band.audibility01);
    }
    
    return (totalMasked / 24.0f) * 100.0f;
}

void AntiMaskingViewModern::rebuildChannelLists()
{
    const int avail = controller.getAvailableInputChannels();

    targetCombo.clear();
    for (auto& cb : maskerCombo)
        cb.clear();

    for (int ch = 0; ch < avail; ++ch)
    {
        const auto name = "Ch " + juce::String(ch + 1);
        targetCombo.addItem(name, ch + 1);
        for (auto& cb : maskerCombo)
            cb.addItem(name, ch + 1);
    }

    targetCombo.setSelectedId(controller.getTargetChannel() + 1, juce::dontSendNotification);

    for (int i = 0; i < 3; ++i)
    {
        maskerCombo[(size_t)i].setSelectedId(controller.getMaskerChannel(i) + 1, 
                                            juce::dontSendNotification);
        maskerEnable[(size_t)i].setToggleState(controller.isMaskerEnabled(i), 
                                              juce::dontSendNotification);
    }
}

void AntiMaskingViewModern::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &targetCombo)
    {
        controller.setTargetChannel(targetCombo.getSelectedId() - 1);
        // Update label to show selected channel
        juce::String channelName = targetCombo.getText();
        targetChannelLabel.setText("Target Channel: **" + channelName + "**", juce::dontSendNotification);
        return;
    }

    for (int i = 0; i < 3; ++i)
    {
        if (comboBoxThatHasChanged == &maskerCombo[(size_t)i])
        {
            controller.setMaskerChannel(i,
                                       maskerCombo[(size_t)i].getSelectedId() - 1,
                                       maskerEnable[(size_t)i].getToggleState());
            return;
        }
    }
}

void AntiMaskingViewModern::buttonClicked(juce::Button* button)
{
    if (button == &autoCorrectButton)
    {
        // TODO: Implement auto-correct functionality
        return;
    }

    if (button == &freezeSceneButton)
    {
        // TODO: Implement freeze scene functionality
        return;
    }

    for (int i = 0; i < 3; ++i)
    {
        if (button == &maskerEnable[(size_t)i])
        {
            controller.setMaskerChannel(i,
                                      maskerCombo[(size_t)i].getSelectedId() - 1,
                                      maskerEnable[(size_t)i].getToggleState());
            return;
        }
    }
}

void AntiMaskingViewModern::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &controller)
        rebuildChannelLists();
    else if (source == &LocalizedStrings::getInstance())
    {
        // Refresh texts when language changes
        auto& strings = LocalizedStrings::getInstance();
        targetChannelLabel.setText(strings.getAntiMaskingTargetLabel(), juce::dontSendNotification);
    }
}

}
