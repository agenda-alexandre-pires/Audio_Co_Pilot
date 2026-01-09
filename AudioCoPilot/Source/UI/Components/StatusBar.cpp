#include "StatusBar.h"
#include "../Colours.h"

StatusBar::StatusBar()
{
}

void StatusBar::paint(juce::Graphics& g)
{
    g.fillAll(Colours::BackgroundDark);
    
    // Linha separadora em cima
    g.setColour(Colours::Border);
    g.drawLine(0.0f, 0.0f, static_cast<float>(getWidth()), 0.0f, 1.0f);
    
    auto bounds = getLocalBounds().reduced(10, 2);
    
    // Status à esquerda
    juce::Colour statusColour;
    switch (_statusType)
    {
        case StatusType::Good:    statusColour = Colours::Safe; break;
        case StatusType::Warning: statusColour = Colours::Warning; break;
        case StatusType::Error:   statusColour = Colours::Alert; break;
        default:                  statusColour = Colours::TextSecondary; break;
    }
    
    // Indicador de status (círculo)
    g.setColour(statusColour);
    g.fillEllipse(static_cast<float>(bounds.getX()), static_cast<float>(bounds.getCentreY() - 4), 8.0f, 8.0f);
    
    // Mensagem
    g.setColour(Colours::TextPrimary);
    g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 12.0f, juce::Font::plain)));
    g.drawText(_statusMessage, bounds.withTrimmedLeft(15), juce::Justification::centredLeft);
    
    // Info técnica à direita
    if (_sampleRate > 0)
    {
        juce::String techInfo = juce::String(_sampleRate / 1000.0f, 1) + "kHz | " +
                                juce::String(_bufferSize) + " samples";
        g.setColour(Colours::TextSecondary);
        g.drawText(techInfo, bounds, juce::Justification::centredRight);
    }
}

void StatusBar::resized()
{
    repaint();
}

void StatusBar::setStatus(const juce::String& message, StatusType type)
{
    _statusMessage = message;
    _statusType = type;
    repaint();
}

void StatusBar::setSampleRateAndBuffer(int sampleRate, int bufferSize)
{
    _sampleRate = sampleRate;
    _bufferSize = bufferSize;
    repaint();
}

