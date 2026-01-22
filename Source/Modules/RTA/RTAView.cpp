#include "RTAView.h"
#include "../../UI/DesignSystem/DesignSystem.h"

namespace AudioCoPilot
{

RTAView::RTAView(RTAController& c, DeviceManager& dm)
    : controller(c)
{
    using namespace DesignSystem;
    
    // Create Device Selector
    deviceSelector = std::make_unique<DeviceSelectorComponent>(dm);
    addAndMakeVisible(deviceSelector.get());
    
    // Resolution Selector
    resolutionLabel.setText("Resolution:", juce::dontSendNotification);
    resolutionLabel.setJustificationType(juce::Justification::centredRight);
    resolutionLabel.setFont(Typography::labelMedium());
    resolutionLabel.setColour(juce::Label::textColourId, Colours::getColour(Colours::Text::Primary));
    addAndMakeVisible(resolutionLabel);
    
    resolutionSelector.addItem("1/3 Octave", 1);
    resolutionSelector.addItem("1/6 Octave", 2);
    resolutionSelector.addItem("1/12 Octave", 3);
    resolutionSelector.addItem("1/24 Octave", 4);
    resolutionSelector.addItem("1/48 Octave", 5);
    
    // Select default
    resolutionSelector.setSelectedId(1);
    
    resolutionSelector.onChange = [this] {
        switch(resolutionSelector.getSelectedId())
        {
            case 1: controller.setResolution(RTAResolution::ThirdOctave); break;
            case 2: controller.setResolution(RTAResolution::SixthOctave); break;
            case 3: controller.setResolution(RTAResolution::TwelfthOctave); break;
            case 4: controller.setResolution(RTAResolution::TwentyFourthOctave); break;
            case 5: controller.setResolution(RTAResolution::FortyEighthOctave); break;
        }
    };
    addAndMakeVisible(resolutionSelector);
    
    // Timer will start when view becomes visible
}

RTAView::~RTAView()
{
    stopTimer();
    deviceSelector = nullptr;
}

void RTAView::timerCallback()
{
    if (! isVisible())
    {
        stopTimer();
        return;
    }
    repaint();
}

void RTAView::visibilityChanged()
{
    if (isVisible())
    {
        if (! isTimerRunning())
            startTimerHz(60);
    }
    else
    {
        if (isTimerRunning())
            stopTimer();
    }
}

void RTAView::paint(juce::Graphics& g)
{
    using namespace DesignSystem;
    g.fillAll(Colours::getColour(Colours::Surface::Background));
    
    // Get levels
    auto leftLevels = controller.getLevels(0);
    auto rightLevels = controller.getLevels(1);
    auto freqs = controller.getFrequencies();
    
    if (freqs.empty()) return;
    
    auto bounds = getLocalBounds();
    // Reserve top area for controls
    bounds.removeFromTop(80);
    
    // Margins
    bounds.reduce(20, 20);
    
    float xStep = (float)bounds.getWidth() / (float)freqs.size();
    float height = (float)bounds.getHeight();
    float bottom = (float)bounds.getBottom();
    
    // Draw Left Channel (Input usually)
    g.setColour(leftColor.withAlpha(0.6f));
    for (size_t i = 0; i < leftLevels.size(); ++i)
    {
        float db = leftLevels[i];
        // Map dB -100 to 0 -> height to 0
        float norm = juce::jmap(db, -100.0f, 0.0f, 0.0f, 1.0f);
        norm = juce::jlimit(0.0f, 1.0f, norm);
        
        float barHeight = height * norm;
        float x = bounds.getX() + i * xStep;
        
        g.fillRect(x, bottom - barHeight, xStep - 1.0f, barHeight);
    }
    
    // Draw Right Channel (or Channel 2)
    // "colors differentes quando entrar dois sinais diferentes"
    // We overlay or put side by side? Overlay is standard RTA.
    g.setColour(rightColor.withAlpha(0.6f));
    for (size_t i = 0; i < rightLevels.size(); ++i)
    {
        float db = rightLevels[i];
        float norm = juce::jmap(db, -100.0f, 0.0f, 0.0f, 1.0f);
        norm = juce::jlimit(0.0f, 1.0f, norm);
        
        float barHeight = height * norm;
        float x = bounds.getX() + i * xStep;
        
        // Draw slightly narrower to see behind? Or just blend mode?
        // Let's draw it over. Alpha blend handles it.
        g.fillRect(x, bottom - barHeight, xStep - 1.0f, barHeight);
    }
}

void RTAView::resized()
{
    auto bounds = getLocalBounds();
    
    auto topBar = bounds.removeFromTop(60);
    
    // Layout Controls
    if (deviceSelector != nullptr)
        deviceSelector->setBounds(topBar.removeFromLeft(topBar.getWidth() / 2).reduced(5));
        
    auto resArea = topBar.removeFromRight(250).reduced(5);
    resolutionLabel.setBounds(resArea.removeFromLeft(100));
    resolutionSelector.setBounds(resArea);
}

}
