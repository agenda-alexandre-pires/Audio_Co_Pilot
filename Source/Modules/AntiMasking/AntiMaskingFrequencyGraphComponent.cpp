#include "AntiMaskingFrequencyGraphComponent.h"
#include <cmath>

namespace AudioCoPilot
{

AntiMaskingFrequencyGraphComponent::AntiMaskingFrequencyGraphComponent()
{
}

void AntiMaskingFrequencyGraphComponent::setAntiMaskingCurve(const std::vector<float>& frequencies,
                                                             const std::vector<float>& gainsDb)
{
    curveFrequencies = frequencies;
    curveGainsDb = gainsDb;
    repaint();
}

void AntiMaskingFrequencyGraphComponent::setMaskingZones(const std::vector<MaskingZone>& zones)
{
    maskingZones = zones;
    repaint();
}

float AntiMaskingFrequencyGraphComponent::frequencyToX(float freq, float width) const
{
    // Logarithmic scale
    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);
    float logFreq = std::log10(juce::jmax(minFreq, juce::jmin(maxFreq, freq)));
    
    return width * (logFreq - logMin) / (logMax - logMin);
}

float AntiMaskingFrequencyGraphComponent::dbToY(float db, float height) const
{
    return height * (1.0f - (db - minDb) / (maxDb - minDb));
}

void AntiMaskingFrequencyGraphComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRect(bounds);
    
    // Graph area with padding
    auto graphArea = bounds.reduced(60, 40);
    
    // Draw grid and axes
    drawGrid(g, graphArea);
    drawFrequencyAxis(g, graphArea);
    drawMagnitudeAxis(g, graphArea);
    
    // Draw masking zones first (behind curve)
    drawMaskingZones(g, graphArea);
    
    // Draw anti-masking curve on top
    drawAntiMaskingCurve(g, graphArea);
}

void AntiMaskingFrequencyGraphComponent::drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    
    // Horizontal grid lines (dB)
    for (float db = minDb; db <= maxDb; db += 18.0f)
    {
        float y = bounds.getY() + dbToY(db, bounds.getHeight());
        g.drawHorizontalLine((int)y, bounds.getX(), bounds.getRight());
    }
    
    // Vertical grid lines (frequency)
    const float freqs[] = {31.5f, 63.0f, 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};
    for (float freq : freqs)
    {
        float x = bounds.getX() + frequencyToX(freq, bounds.getWidth());
        g.drawVerticalLine((int)x, bounds.getY(), bounds.getBottom());
    }
}

void AntiMaskingFrequencyGraphComponent::drawFrequencyAxis(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(10.0f));
    
    const float freqs[] = {20.0f, 100.0f, 500.0f, 1000.0f, 5000.0f, 10000.0f, 20000.0f};
    const juce::String labels[] = {"20HZ", "100HZ", "500HZ", "1KHZ", "5KHZ", "10KHZ", "20KHZ"};
    
    auto axisArea = bounds.removeFromBottom(25);
    
    for (size_t i = 0; i < 7; ++i)
    {
        float x = bounds.getX() + frequencyToX(freqs[i], bounds.getWidth());
        g.drawText(labels[i], 
                  (int)(x - 30), (int)axisArea.getY(), 60, (int)axisArea.getHeight(),
                  juce::Justification::centred);
    }
}

void AntiMaskingFrequencyGraphComponent::drawMagnitudeAxis(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(10.0f));
    
    auto axisArea = bounds.removeFromLeft(50);
    
    for (float db = minDb; db <= maxDb; db += 18.0f)
    {
        float y = bounds.getY() + dbToY(db, bounds.getHeight());
        g.drawText(juce::String((int)db) + " dB",
                  (int)axisArea.getX(), (int)(y - 7), (int)axisArea.getWidth(), 14,
                  juce::Justification::centredRight);
    }
}

void AntiMaskingFrequencyGraphComponent::drawMaskingZones(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Draw brown masking zones (semi-transparent)
    g.setColour(juce::Colour(0x99663333).withAlpha(0.4f));
    
    for (const auto& zone : maskingZones)
    {
        float centerX = bounds.getX() + frequencyToX(zone.centerFreq, bounds.getWidth());
        float bandwidthHz = zone.centerFreq * (std::pow(2.0f, zone.bandwidth) - 1.0f);
        float leftFreq = zone.centerFreq - bandwidthHz * 0.5f;
        float rightFreq = zone.centerFreq + bandwidthHz * 0.5f;
        
        float leftX = bounds.getX() + frequencyToX(leftFreq, bounds.getWidth());
        float rightX = bounds.getX() + frequencyToX(rightFreq, bounds.getWidth());
        float width = rightX - leftX;
        
        // Bell-shaped zone
        juce::Path zonePath;
        float centerY = bounds.getCentreY();
        float height = bounds.getHeight() * zone.severity * 0.6f;
        
        zonePath.startNewSubPath(leftX, bounds.getBottom());
        zonePath.quadraticTo(centerX, centerY - height, rightX, bounds.getBottom());
        zonePath.quadraticTo(centerX, centerY - height, leftX, bounds.getBottom());
        zonePath.closeSubPath();
        
        g.fillPath(zonePath);
    }
}

void AntiMaskingFrequencyGraphComponent::drawAntiMaskingCurve(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    if (curveFrequencies.empty() || curveGainsDb.empty())
        return;
    
    // Draw smooth blue curve
    juce::Path curve;
    bool first = true;
    
    for (size_t i = 0; i < curveFrequencies.size() && i < curveGainsDb.size(); ++i)
    {
        float x = bounds.getX() + frequencyToX(curveFrequencies[i], bounds.getWidth());
        float y = bounds.getY() + dbToY(curveGainsDb[i], bounds.getHeight());
        
        if (first)
        {
            curve.startNewSubPath(x, y);
            first = false;
        }
        else
        {
            curve.lineTo(x, y);
        }
    }
    
    // Draw curve with blue color
    g.setColour(juce::Colour(0xff3b82f6)); // Blue
    g.strokePath(curve, juce::PathStrokeType(2.5f));
}

void AntiMaskingFrequencyGraphComponent::resized()
{
    repaint();
}

}
