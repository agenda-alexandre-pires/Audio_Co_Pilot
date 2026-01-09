#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Core/AudioEngine.h"

class MeterBridge : public juce::Component,
                     public juce::Button::Listener
{
public:
    MeterBridge();
    ~MeterBridge() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;

    void setLevels(const std::vector<ChannelLevels>& levels);
    void clearPeakHold();

private:
    std::vector<ChannelLevels> _levels;
    
    // Estado do display para animação (em LINEAR, não dB)
    std::vector<float> _displayPeak;
    std::vector<float> _displayRms;
    std::vector<bool> _displayClip;
    
    // Peak hold (valores máximos mantidos até reset)
    std::vector<float> _peakHoldValues;  // Em dB
    
    // Botão para limpar peak hold
    juce::TextButton _clearPeakHoldButton;
    
    // Coeficiente de decay (calculado uma vez)
    float _decayCoeff = 0.0f;
    
    void drawMeter(juce::Graphics& g, juce::Rectangle<float> bounds,
                   float peakDB, float rmsDB, bool clipping, int channelNum);
    
    float dbToY(float db, float height) const;
    juce::Colour getColourForLevel(float db) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeterBridge)
};

