#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "MaskingCalculator.h"
#include <array>

namespace AudioCoPilot
{
    /**
     * @brief Display visual da matriz de mascaramento
     * 
     * Mostra um heatmap colorido indicando a audibilidade do target
     * em cada banda Bark, com indicação dos maskers dominantes.
     */
    class MaskingMatrixDisplay : public juce::Component
    {
    public:
        MaskingMatrixDisplay();
        ~MaskingMatrixDisplay() override = default;
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        /**
         * @brief Atualiza o display com novo resultado
         */
        void setMaskingResult(const MaskingAnalysisResult& result);
        
        /**
         * @brief Define as cores dos maskers
         */
        void setMaskerColours(juce::Colour m1, juce::Colour m2, juce::Colour m3);
        
    private:
        MaskingAnalysisResult _currentResult;
        
        std::array<juce::Colour, 3> _maskerColours = {
            juce::Colour(0xFF00AAFF),  // Azul
            juce::Colour(0xFFFF00AA),  // Magenta
            juce::Colour(0xFFAAFF00)   // Verde-limão
        };
        
        // Calcula cor baseada na audibilidade
        juce::Colour getAudibilityColour(float audibility, int dominantMasker);
        
        // Desenha uma banda
        void drawBand(juce::Graphics& g, int bandIndex, juce::Rectangle<float> bounds);
        
        // Cache de rects
        std::array<juce::Rectangle<float>, 24> _bandRects;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MaskingMatrixDisplay)
    };
}