#include "MaskingMatrixDisplay.h"
#include "BarkAnalyzer.h"
#include "../../UI/Colours.h"

namespace AudioCoPilot
{
    MaskingMatrixDisplay::MaskingMatrixDisplay()
    {
    }
    
    void MaskingMatrixDisplay::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(Colours::BackgroundDark);
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Borda
        g.setColour(Colours::Border);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        
        // Área do gráfico
        auto graphBounds = bounds.reduced(10.0f);
        graphBounds.removeFromTop(25.0f);    // Espaço para título
        graphBounds.removeFromBottom(30.0f); // Espaço para labels
        
        // Título
        g.setColour(Colours::TextPrimary);
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 14.0f, juce::Font::bold)));
        g.drawText("MASKING MATRIX", bounds.removeFromTop(25.0f), juce::Justification::centred);
        
        // Desenha cada banda
        float bandWidth = graphBounds.getWidth() / 24.0f;
        
        for (int i = 0; i < 24; ++i)
        {
            auto bandRect = graphBounds.withWidth(bandWidth).withX(graphBounds.getX() + i * bandWidth);
            _bandRects[i] = bandRect;
            drawBand(g, i, bandRect);
        }
        
        // Labels de frequência
        g.setColour(Colours::TextSecondary);
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 9.0f, juce::Font::plain)));
        
        // Mostra algumas frequências chave
        std::array<int, 6> labelBands = {0, 4, 9, 14, 19, 23};
        for (int band : labelBands)
        {
            float freq = BarkAnalyzer::getBandCenterHz(band);
            juce::String label;
            if (freq >= 1000)
                label = juce::String(freq / 1000.0f, 1) + "k";
            else
                label = juce::String(static_cast<int>(freq));
            
            auto rect = _bandRects[band];
            g.drawText(label, 
                       static_cast<int>(rect.getCentreX() - 15), 
                       static_cast<int>(graphBounds.getBottom() + 5),
                       30, 20, juce::Justification::centred);
        }
        
        // Escala de audibilidade à direita
        auto scaleBounds = bounds.removeFromRight(60.0f).reduced(5.0f, 30.0f);
        
        // Gradiente
        juce::ColourGradient gradient(
            Colours::Safe, scaleBounds.getBottomLeft(),
            Colours::Alert, scaleBounds.getTopLeft(), false);
        gradient.addColour(0.5, Colours::Warning);
        
        g.setGradientFill(gradient);
        g.fillRect(scaleBounds.withWidth(15.0f));
        
        // Labels da escala
        g.setColour(Colours::TextSecondary);
        g.drawText("Clear", scaleBounds.withY(scaleBounds.getBottom() - 15).withHeight(15), 
                   juce::Justification::left);
        g.drawText("Masked", scaleBounds.withHeight(15), juce::Justification::left);
    }
    
    void MaskingMatrixDisplay::resized()
    {
        repaint();
    }
    
    void MaskingMatrixDisplay::setMaskingResult(const MaskingAnalysisResult& result)
    {
        _currentResult = result;
        repaint();
    }
    
    void MaskingMatrixDisplay::setMaskerColours(juce::Colour m1, juce::Colour m2, juce::Colour m3)
    {
        _maskerColours[0] = m1;
        _maskerColours[1] = m2;
        _maskerColours[2] = m3;
        repaint();
    }
    
    juce::Colour MaskingMatrixDisplay::getAudibilityColour(float audibility, int dominantMasker)
    {
        // Base: verde (audível) para vermelho (mascarado)
        juce::Colour baseColour;
        
        if (audibility > 0.7f)
            baseColour = Colours::Safe;
        else if (audibility > 0.4f)
            baseColour = Colours::Warning;
        else
            baseColour = Colours::Alert;
        
        // Se há masker dominante, mistura com a cor do masker
        if (dominantMasker >= 0 && dominantMasker < 3 && audibility < 0.7f)
        {
            float blendAmount = 1.0f - audibility;
            return baseColour.interpolatedWith(_maskerColours[dominantMasker], blendAmount * 0.5f);
        }
        
        return baseColour;
    }
    
    void MaskingMatrixDisplay::drawBand(juce::Graphics& g, int bandIndex, juce::Rectangle<float> bounds)
    {
        const auto& bandResult = _currentResult.bandResults[bandIndex];
        
        // Cor baseada na audibilidade
        auto colour = getAudibilityColour(bandResult.audibility, bandResult.dominantMasker);
        
        // Altura proporcional à audibilidade
        float height = bounds.getHeight() * (1.0f - bandResult.audibility);
        auto fillRect = bounds.withTop(bounds.getBottom() - height).reduced(1.0f, 0.0f);
        
        // Preenche
        g.setColour(colour.withAlpha(0.8f));
        g.fillRect(fillRect);
        
        // Borda da banda
        g.setColour(Colours::Border.withAlpha(0.5f));
        g.drawRect(bounds.reduced(0.5f));
        
        // Indicador de nível do target (linha branca)
        if (bandResult.targetLevel > -90.0f)
        {
            float targetY = juce::jmap(bandResult.targetLevel, -60.0f, 0.0f, 
                                       bounds.getBottom(), bounds.getY());
            targetY = juce::jlimit(bounds.getY(), bounds.getBottom(), targetY);
            
            g.setColour(juce::Colours::white);
            g.drawHorizontalLine(static_cast<int>(targetY), bounds.getX(), bounds.getRight());
        }
    }
}