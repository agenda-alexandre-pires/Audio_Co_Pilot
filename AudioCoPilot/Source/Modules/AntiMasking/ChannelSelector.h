#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace AudioCoPilot
{
    /**
     * @brief Seletor de canais para o módulo Anti-Masking
     * 
     * Permite selecionar qual canal é o target e quais são os maskers,
     * com cores diferentes para cada tipo.
     */
    class ChannelSelector : public juce::Component,
                            public juce::Button::Listener,
                            public juce::ComboBox::Listener
    {
    public:
        ChannelSelector();
        ~ChannelSelector() override = default;
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
        
        /**
         * @brief Define o número de canais disponíveis
         */
        void setNumChannels(int numChannels);
        
        /**
         * @brief Retorna o canal selecionado como target
         */
        int getTargetChannel() const { return _targetChannel; }
        
        /**
         * @brief Retorna se um masker está habilitado
         */
        bool isMaskerEnabled(int maskerIndex) const;
        
        /**
         * @brief Retorna o canal de um masker
         */
        int getMaskerChannel(int maskerIndex) const;
        
        /**
         * @brief Define um listener para mudanças
         */
        void setListener(std::function<void()> listener);
        
    private:
        int _numChannels = 4;
        int _targetChannel = 0;
        
        struct MaskerConfig
        {
            bool enabled = false;
            int channel = 1;
        };
        
        std::array<MaskerConfig, 3> _maskers;
        
        // UI elements
        std::unique_ptr<juce::Label> _titleLabel;
        std::unique_ptr<juce::Label> _targetLabel;
        std::array<std::unique_ptr<juce::ComboBox>, 4> _channelSelectors;
        std::array<std::unique_ptr<juce::ToggleButton>, 3> _maskerToggles;
        std::array<std::unique_ptr<juce::Label>, 3> _maskerLabels;
        
        std::function<void()> _changeListener;
        
        void updateChannelSelectors();
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelSelector)
    };
}