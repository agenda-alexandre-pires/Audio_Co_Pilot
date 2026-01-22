#include "PhasePlotComponent.h"
#include <cmath>

PhasePlotComponent::PhasePlotComponent(TFProcessor& proc)
    : processor(proc)
{
    // Start timer for real-time updates (30Hz = 33ms for stable Smaart-like display)
    // Reduced from 60Hz to avoid jitter and improve stability
    startTimer(33);
}

PhasePlotComponent::~PhasePlotComponent()
{
    stopTimer();
}

void PhasePlotComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Background
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(bounds);
    
    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("Phase Response", bounds.removeFromTop(25), juce::Justification::centredLeft);
    
    // Draw graph
    drawPhaseGraph(g, bounds);
}

void PhasePlotComponent::drawPhaseGraph(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Get latest data
    processor.getPhaseResponse(phaseData);
    processor.getFrequencyBins(frequencies);
    processor.getCoherence(coherenceData);
    
    if (phaseData.empty() || frequencies.empty() || phaseData.size() != frequencies.size())
        return;
    
    // Ensure coherence data matches
    if (coherenceData.size() != phaseData.size())
        coherenceData.resize(phaseData.size(), 1.0f);
    
    const float padding = 40.0f;
    auto graphArea = bounds.reduced(static_cast<int>(padding));
    float width = static_cast<float>(graphArea.getWidth());
    float height = static_cast<float>(graphArea.getHeight());
    
    // Draw axes
    g.setColour(juce::Colour(0xff404040));  // Darker gray for axes
    
    // Horizontal axis (frequency, log scale)
    g.drawLine(padding, graphArea.getBottom(), graphArea.getRight(), graphArea.getBottom(), 1.0f);
    
    // Vertical axis (phase)
    g.drawLine(padding, graphArea.getY(), padding, graphArea.getBottom(), 1.0f);
    
    // Frequency labels (Smaart-style: 31.5, 63, 125, 250, 500, 1k, 2k, 4k, 8k, 16k)
    // CRITICAL: Use the SAME frequencyToX function for ticks AND plot
    g.setFont(juce::Font(10.0f));
    g.setColour(juce::Colour(0xff505050));  // Softer gray for labels
    const float ticks[] = {31.5f, 63.0f, 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};
    const juce::String tickLabels[] = {"31.5", "63", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"};
    
    for (int i = 0; i < 10; ++i)
    {
        float freq = ticks[i];
        if (freq < minFrequency || freq > maxFrequency)
            continue;
        
        // Use the SAME frequencyToX function that the plot uses
        float x = frequencyToX(freq, width, minFrequency, maxFrequency);
        
        // Draw vertical line at this position (softer gray, less visible)
        g.setColour(juce::Colour(0xff353535));  // Very soft gray for grid lines
        g.drawVerticalLine(static_cast<int>(padding + x), graphArea.getY(), graphArea.getBottom());
        g.setColour(juce::Colour(0xff505050));  // Restore label color
        
        // Draw label at the same position
        g.drawText(tickLabels[i], 
                   static_cast<int>(padding + x - 20), graphArea.getBottom() + 2, 40, 15,
                   juce::Justification::centred);
    }
    
    // Phase labels
    for (float phase = -180.0f; phase <= 180.0f; phase += 90.0f)
    {
        float y = phaseToY(phase, height);
        // Draw horizontal grid line (softer gray, less visible)
        g.setColour(juce::Colour(0xff353535));  // Very soft gray for grid lines
        g.drawHorizontalLine(static_cast<int>(graphArea.getY() + y), 
                            graphArea.getX(), graphArea.getRight());
        g.setColour(juce::Colour(0xff505050));  // Softer gray for labels
        g.drawText(juce::String(static_cast<int>(phase)) + "°", 
                   5, static_cast<int>(graphArea.getY() + y - 7), 35, 14,
                   juce::Justification::centredRight);
    }
    
    // Draw phase curve with coherence-based alpha (Smaart-style)
    if (phaseData.size() < 2)
        return;
    
    // Draw in segments based on coherence
    juce::Path currentSegment;
    bool segmentStarted = false;
    
    for (size_t i = 0; i < phaseData.size(); ++i)
    {
        float freq = frequencies[i];
        if (freq < minFrequency || freq > maxFrequency)
            continue;
        
        float coh = (i < coherenceData.size()) ? coherenceData[i] : 1.0f;
        
        // Calculate alpha based on coherence (Smaart-style)
        // alpha = clamp((coh - 0.5)/0.5, 0, 1)
        float alpha = juce::jlimit(0.0f, 1.0f, (coh - 0.5f) / 0.5f);
        
        // Skip points with very low coherence (almost invisible)
        if (alpha < 0.1f)
        {
            // Break path segment
            if (segmentStarted)
            {
                g.setColour(graphColour.withAlpha(0.4f));
                g.strokePath(currentSegment, juce::PathStrokeType(2.5f));  // Thicker line
                currentSegment.clear();
                segmentStarted = false;
            }
            continue;
        }
        
        float x = frequencyToX(freq, width, minFrequency, maxFrequency);
        float y = phaseToY(phaseData[i], height);
        
        float graphX = padding + x;
        float graphY = graphArea.getY() + y;
        
        if (!segmentStarted)
        {
            currentSegment.startNewSubPath(graphX, graphY);
            segmentStarted = true;
        }
        else
        {
            currentSegment.lineTo(graphX, graphY);
        }
        
        // Draw segment when coherence changes significantly or at end
        if (i == phaseData.size() - 1 || 
            (i > 0 && std::abs(alpha - juce::jlimit(0.0f, 1.0f, (coherenceData[i-1] - 0.5f) / 0.5f)) > 0.3f))
        {
            // Stronger color with thicker line (2.5px instead of 1.5px)
            g.setColour(graphColour.withAlpha(0.5f + alpha * 0.5f));  // Range: 0.5 to 1.0 (stronger)
            g.strokePath(currentSegment, juce::PathStrokeType(2.5f));  // Thicker line
            currentSegment.clear();
            segmentStarted = false;
        }
    }
    
    // Draw final segment if any
    if (segmentStarted)
    {
        g.setColour(graphColour);  // Full intensity for final segment
        g.strokePath(currentSegment, juce::PathStrokeType(2.5f));  // Thicker line
    }
}

float PhasePlotComponent::frequencyToX(float frequency, float width, float minFreq, float maxFreq)
{
    // Logarithmic scale
    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);
    float logFreq = std::log10(frequency);
    
    return width * (logFreq - logMin) / (logMax - logMin);
}

float PhasePlotComponent::phaseToY(float phaseDegrees, float height)
{
    // Linear scale, -180° at top, +180° at bottom
    float normalized = (phaseDegrees - minPhase) / (maxPhase - minPhase);
    return height * (1.0f - normalized);
}

void PhasePlotComponent::resized()
{
    repaint();
}

void PhasePlotComponent::timerCallback()
{
    // Only repaint if visible (optimized for real-time performance)
    if (!isVisible())
    {
        stopTimer();
        return;
    }
    
    // Repaint for real-time updates (60Hz = smooth Smaart-like display)
    repaint();
}

void PhasePlotComponent::visibilityChanged()
{
    if (isVisible())
    {
        if (!isTimerRunning())
            startTimer(33);  // ~30Hz (33ms) for stable real-time display
    }
    else
    {
        if (isTimerRunning())
            stopTimer();
    }
}
