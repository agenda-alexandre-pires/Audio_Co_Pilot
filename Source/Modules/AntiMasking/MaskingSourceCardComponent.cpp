#include "MaskingSourceCardComponent.h"

namespace AudioCoPilot
{

MaskingSourceCardComponent::MaskingSourceCardComponent()
{
    spectrumDb.fill(-100.0f);
}

void MaskingSourceCardComponent::setSourceInfo(int index,
                                               const juce::String& name,
                                               float levelDb,
                                               float maskingImpactPercent,
                                               const std::array<float, 24>& spectrumDb)
{
    sourceIndex = index;
    sourceName = name;
    this->levelDb = levelDb;
    this->maskingImpactPercent = maskingImpactPercent;
    this->spectrumDb = spectrumDb;
    
    // Determine impact level
    if (maskingImpactPercent >= 15.0f)
        impactLevel = ImpactLevel::High;
    else if (maskingImpactPercent >= 8.0f)
        impactLevel = ImpactLevel::Moderate;
    else
        impactLevel = ImpactLevel::Low;
    
    repaint();
}

void MaskingSourceCardComponent::setImpactLevel(ImpactLevel level)
{
    impactLevel = level;
    repaint();
}

void MaskingSourceCardComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Card background - dark gray with border
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRoundedRectangle(bounds, cardCornerRadius);
    
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawRoundedRectangle(bounds, cardCornerRadius, 1.0f);
    
    // Indicator circle (left)
    auto indicatorArea = bounds.removeFromLeft(40);
    g.setColour(indicatorColour);
    g.fillEllipse(indicatorArea.reduced(12, 12));
    
    bounds.removeFromLeft(10);
    
    // Label area
    auto labelArea = bounds.removeFromTop(25);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(juce::String::formatted("%02d. %s", sourceIndex + 1, sourceName.toRawUTF8()),
               labelArea, juce::Justification::centredLeft);
    
    // Level
    g.setFont(juce::Font(10.0f));
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawText("LVL: " + juce::String(levelDb, 1) + "dB",
               bounds.removeFromTop(18), juce::Justification::centredLeft);
    
    // Mini graph area
    auto graphArea = bounds.removeFromTop(60).reduced(5, 5);
    drawMiniGraph(g, graphArea);
    
    // Masking Impact
    auto impactArea = bounds.removeFromTop(30);
    g.setFont(juce::Font(9.0f));
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.drawText("MASKING IMPACT", impactArea.removeFromTop(14), juce::Justification::centredLeft);
    
    // Impact value with color coding
    juce::Colour impactColour;
    juce::String impactText;
    
    switch (impactLevel)
    {
        case ImpactLevel::High:
            impactColour = juce::Colour(0xffff6b35); // Orange
            impactText = "HIGH (" + juce::String(maskingImpactPercent, 1) + "%)";
            break;
        case ImpactLevel::Moderate:
            impactColour = juce::Colour(0xff4ade80); // Green
            impactText = "MODERATE (" + juce::String(maskingImpactPercent, 1) + "%)";
            break;
        case ImpactLevel::Low:
            impactColour = juce::Colour(0xff60a5fa); // Light blue
            impactText = "LOW (" + juce::String(maskingImpactPercent, 1) + "%)";
            break;
    }
    
    g.setColour(impactColour);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(impactText, impactArea, juce::Justification::centredLeft);
}

void MaskingSourceCardComponent::drawMiniGraph(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Background
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(bounds, 2.0f);
    
    // Graph area
    auto graphBounds = bounds.reduced(2.0f);
    
    const float minDb = -60.0f;
    const float maxDb = 0.0f;
    const float bandW = graphBounds.getWidth() / 24.0f;
    
    // Draw spectrum bars
    for (int b = 0; b < 24; ++b)
    {
        const float db = juce::jlimit(minDb, maxDb, spectrumDb[(size_t)b]);
        const float h = juce::jmap(db, minDb, maxDb, 0.0f, graphBounds.getHeight());
        
        auto bar = juce::Rectangle<float>(
            graphBounds.getX() + b * bandW + 1.0f,
            graphBounds.getBottom() - h,
            bandW - 2.0f,
            h
        );
        
        // Brown masking zone (semi-transparent)
        g.setColour(juce::Colour(0x99663333).withAlpha(0.4f));
        g.fillRect(bar);
        
        // Spectrum curve (white outline)
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.drawRect(bar, 0.5f);
    }
    
    // Border
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
}

void MaskingSourceCardComponent::resized()
{
    // Layout handled in paint
}

}
