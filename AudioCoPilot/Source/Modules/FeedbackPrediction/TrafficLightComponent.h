#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "HowlingDetector.h"

namespace AudioCoPilot {

class TrafficLightComponent : public juce::Component,
                               public juce::Timer {
public:
    TrafficLightComponent();
    ~TrafficLightComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    void setStatus(FeedbackCandidate::Status status);
    void setFrequency(float freq);
    void setRiskScore(float score);
    
private:
    FeedbackCandidate::Status _status = FeedbackCandidate::Status::Safe;
    float _frequency = 0.0f;
    float _riskScore = 0.0f;
    float _pulsePhase = 0.0f;
    
    juce::Colour getStatusColour() const;
    juce::String getStatusText() const;
};

} // namespace AudioCoPilot

