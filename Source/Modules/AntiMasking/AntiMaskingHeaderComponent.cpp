#include "AntiMaskingHeaderComponent.h"
#include "../../UI/DesignSystem/DesignSystem.h"

namespace AudioCoPilot
{

AntiMaskingHeaderComponent::AntiMaskingHeaderComponent()
{
    // App name and logo
    appNameLabel.setText("AUDIO CO PILOT", juce::dontSendNotification);
    appNameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    appNameLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    addAndMakeVisible(appNameLabel);
    
    // Status labels
    cpuLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4ade80)); // Green
    cpuLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(cpuLabel);
    
    sampleRateLabel.setColour(juce::Label::textColourId, juce::Colour(0xff60a5fa)); // Blue
    sampleRateLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(sampleRateLabel);
    
    setCPUUsage(0.0f);
    setSampleRate(48000, 24);
}

void AntiMaskingHeaderComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background - dark theme
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRect(bounds);
    
    // Bottom border
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(bounds.getX(), bounds.getBottom(), bounds.getRight(), bounds.getBottom(), 1.0f);
}

void AntiMaskingHeaderComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Left: App name
    auto leftArea = bounds.removeFromLeft(200);
    appNameLabel.setBounds(leftArea.reduced(10, 0));
    
    // Right: Status indicators
    auto rightArea = bounds.removeFromRight(300);
    
    // CPU
    auto cpuArea = rightArea.removeFromLeft(120);
    cpuLabel.setBounds(cpuArea.reduced(5, 0));
    
    // Sample rate
    sampleRateLabel.setBounds(rightArea.reduced(5, 0));
}

void AntiMaskingHeaderComponent::setCPUUsage(float cpuPercent)
{
    cpuUsage = cpuPercent;
    cpuLabel.setText("CPU " + juce::String(cpuPercent, 1) + "%", juce::dontSendNotification);
}

void AntiMaskingHeaderComponent::setSampleRate(int sr, int bits)
{
    sampleRate = sr;
    bitDepth = bits;
    sampleRateLabel.setText(juce::String(sr / 1000) + "kHz / " + juce::String(bits) + "bit", 
                           juce::dontSendNotification);
}

}
