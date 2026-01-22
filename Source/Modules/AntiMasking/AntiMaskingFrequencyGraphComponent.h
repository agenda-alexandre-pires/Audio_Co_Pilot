#pragma once

#include "../../JuceHeader.h"
#include <array>
#include <vector>

namespace AudioCoPilot
{

/**
 * AntiMaskingFrequencyGraphComponent
 * 
 * Gráfico principal de frequência (20Hz-20kHz)
 * Mostra curva azul de anti-masking e zonas marrons de masking
 */
class AntiMaskingFrequencyGraphComponent : public juce::Component
{
public:
    AntiMaskingFrequencyGraphComponent();
    ~AntiMaskingFrequencyGraphComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Set anti-masking curve (EQ curve)
    void setAntiMaskingCurve(const std::vector<float>& frequencies, 
                            const std::vector<float>& gainsDb);
    
    // Set masking zones (frequency ranges with masking)
    struct MaskingZone
    {
        float centerFreq;
        float bandwidth; // octaves
        float severity; // 0-1
    };
    void setMaskingZones(const std::vector<MaskingZone>& zones);

private:
    void drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawFrequencyAxis(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawMagnitudeAxis(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawAntiMaskingCurve(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawMaskingZones(juce::Graphics& g, juce::Rectangle<float> bounds);
    
    float frequencyToX(float freq, float width) const;
    float dbToY(float db, float height) const;
    
    std::vector<float> curveFrequencies;
    std::vector<float> curveGainsDb;
    std::vector<MaskingZone> maskingZones;
    
    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minDb = -64.0f;
    static constexpr float maxDb = 0.0f;
};

}
