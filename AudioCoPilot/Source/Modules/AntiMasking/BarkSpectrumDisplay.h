#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

namespace AudioCoPilot
{
    /**
     * @brief Display de espectro Bark individual
     * 
     * Mostra o espectro de um canal em formato de barras nas 24 bandas Bark.
     */
    class BarkSpectrumDisplay : public juce::Component
    {
    public:
        BarkSpectrumDisplay();
        ~BarkSpectrumDisplay() override = default;
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        /**
         * @brief Atualiza o espectro exibido
         */
        void setSpectrum(const std::array<float, 24>& spectrum);
        
        /**
         * @brief Define a cor do canal
         */
        void setChannelColour(juce::Colour colour);
        
        /**
         * @brief Define o nome do canal
         */
        void setChannelName(const juce::String& name);
        
        /**
         * @brief Define o range em dB
         */
        void setDBRange(float minDB, float maxDB);
        
    private:
        std::array<float, 24> _spectrum;
        juce::Colour _channelColour;
        juce::String _channelName;
        float _minDB = -60.0f;
        float _maxDB = 0.0f;
        
        float dbToY(float db, float height) const;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BarkSpectrumDisplay)
    };
}