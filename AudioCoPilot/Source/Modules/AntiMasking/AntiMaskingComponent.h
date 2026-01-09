#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "AntiMaskingProcessor.h"
#include "MaskingMatrixDisplay.h"
#include "BarkSpectrumDisplay.h"
#include "ChannelSelector.h"

namespace AudioCoPilot
{
    /**
     * @brief Componente UI principal do módulo Anti-Masking
     */
    class AntiMaskingComponent : public juce::Component,
                                  public juce::Timer
    {
    public:
        AntiMaskingComponent();
        ~AntiMaskingComponent() override;
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        void timerCallback() override;
        
        /**
         * @brief Conecta ao processador
         */
        void setProcessor(AntiMaskingProcessor* processor);
        
    private:
        AntiMaskingProcessor* _processor = nullptr;
        
        // Sub-componentes
        std::unique_ptr<MaskingMatrixDisplay> _matrixDisplay;
        std::unique_ptr<BarkSpectrumDisplay> _targetSpectrum;
        std::unique_ptr<BarkSpectrumDisplay> _maskerSpectrum1;
        std::unique_ptr<BarkSpectrumDisplay> _maskerSpectrum2;
        std::unique_ptr<BarkSpectrumDisplay> _maskerSpectrum3;
        std::unique_ptr<ChannelSelector> _channelSelector;
        
        // Labels
        std::unique_ptr<juce::Label> _titleLabel;
        std::unique_ptr<juce::Label> _audibilityLabel;
        std::unique_ptr<juce::Label> _criticalBandsLabel;
        
        void updateDisplay();
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AntiMaskingComponent)
    };
}