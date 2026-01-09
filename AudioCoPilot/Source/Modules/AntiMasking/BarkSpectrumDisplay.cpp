#include "BarkSpectrumDisplay.h"
#include "BarkAnalyzer.h"
#include "../../UI/Colours.h"

namespace AudioCoPilot
{
    BarkSpectrumDisplay::BarkSpectrumDisplay()
        : _channelColour(Colours::Accent)
    {
        _spectrum.fill(-100.0f);
    }
    
    void BarkSpectrumDisplay::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(Colours::BackgroundDark);
        g.fillRoundedRectangle(bounds, 3.0f);
        
        // Área do gráfico
        auto graphBounds = bounds.reduced(5.0f);
        graphBounds.removeFromTop(18.0f); // Espaço para nome
        
        // Nome do canal
        g.setColour(_channelColour);
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 11.0f, juce::Font::bold)));
        g.drawText(_channelName, bounds.withHeight(18.0f), juce::Justification::centred);
        
        // Barras do espectro
        float bandWidth = graphBounds.getWidth() / 24.0f;
        
        for (int i = 0; i < 24; ++i)
        {
            float level = _spectrum[i];
            float height = dbToY(level, graphBounds.getHeight());
            
            auto barRect = juce::Rectangle<float>(
                graphBounds.getX() + i * bandWidth + 1.0f,
                graphBounds.getBottom() - height,
                bandWidth - 2.0f,
                height
            );
            
            // Gradiente para a barra
            juce::ColourGradient gradient(
                _channelColour.brighter(0.3f), barRect.getTopLeft(),
                _channelColour.darker(0.3f), barRect.getBottomLeft(), false);
            
            g.setGradientFill(gradient);
            g.fillRect(barRect);
        }
        
        // Grid lines
        g.setColour(Colours::Border.withAlpha(0.3f));
        for (float db = _minDB; db <= _maxDB; db += 12.0f)
        {
            float y = graphBounds.getBottom() - dbToY(db, graphBounds.getHeight());
            g.drawHorizontalLine(static_cast<int>(y), graphBounds.getX(), graphBounds.getRight());
        }
        
        // Borda
        g.setColour(Colours::Border);
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
    }
    
    void BarkSpectrumDisplay::resized()
    {
        repaint();
    }
    
    void BarkSpectrumDisplay::setSpectrum(const std::array<float, 24>& spectrum)
    {
        _spectrum = spectrum;
        repaint();
    }
    
    void BarkSpectrumDisplay::setChannelColour(juce::Colour colour)
    {
        _channelColour = colour;
        repaint();
    }
    
    void BarkSpectrumDisplay::setChannelName(const juce::String& name)
    {
        _channelName = name;
        repaint();
    }
    
    void BarkSpectrumDisplay::setDBRange(float minDB, float maxDB)
    {
        _minDB = minDB;
        _maxDB = maxDB;
        repaint();
    }
    
    float BarkSpectrumDisplay::dbToY(float db, float height) const
    {
        db = juce::jlimit(_minDB, _maxDB, db);
        return height * (db - _minDB) / (_maxDB - _minDB);
    }
}