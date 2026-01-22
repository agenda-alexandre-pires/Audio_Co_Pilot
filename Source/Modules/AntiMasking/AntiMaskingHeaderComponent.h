#pragma once

#include "../../JuceHeader.h"

namespace AudioCoPilot
{

/**
 * AntiMaskingHeaderComponent
 * 
 * Header moderno com navegação, logo e status indicators
 * Design baseado no Stitch apresentado
 */
class AntiMaskingHeaderComponent : public juce::Component
{
public:
    AntiMaskingHeaderComponent();
    ~AntiMaskingHeaderComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Status updates
    void setCPUUsage(float cpuPercent);
    void setSampleRate(int sampleRate, int bitDepth);

private:
    juce::Label appNameLabel;
    juce::ImageComponent logoComponent;
    
    // Status indicators
    juce::Label cpuLabel;
    juce::Label sampleRateLabel;
    
    float cpuUsage{0.0f};
    int sampleRate{48000};
    int bitDepth{24};
    
    static constexpr int headerHeight = 60;
};

}
