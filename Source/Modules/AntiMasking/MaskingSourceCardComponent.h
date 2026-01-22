#pragma once

#include "../../JuceHeader.h"
#include <array>

namespace AudioCoPilot
{

/**
 * MaskingSourceCardComponent
 * 
 * Card modular para exibir uma fonte de masking
 * Design baseado no Stitch: indicador colorido, label, level, mini-graph, impact
 */
class MaskingSourceCardComponent : public juce::Component
{
public:
    MaskingSourceCardComponent();
    ~MaskingSourceCardComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Configuração do card
    void setSourceInfo(int index, 
                      const juce::String& name,
                      float levelDb,
                      float maskingImpactPercent,
                      const std::array<float, 24>& spectrumDb);

    void setColour(juce::Colour c) { indicatorColour = c; repaint(); }
    
    enum class ImpactLevel
    {
        Low,
        Moderate,
        High
    };
    
    void setImpactLevel(ImpactLevel level);

private:
    void drawMiniGraph(juce::Graphics& g, juce::Rectangle<float> bounds);
    
    int sourceIndex{0};
    juce::String sourceName;
    float levelDb{-100.0f};
    float maskingImpactPercent{0.0f};
    ImpactLevel impactLevel{ImpactLevel::Low};
    juce::Colour indicatorColour{juce::Colours::red};
    
    std::array<float, 24> spectrumDb;
    
    static constexpr float cardCornerRadius = 6.0f;
};

}
