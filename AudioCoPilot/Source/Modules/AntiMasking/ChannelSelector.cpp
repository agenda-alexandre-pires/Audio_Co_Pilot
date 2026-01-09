#include "ChannelSelector.h"
#include "../../UI/Colours.h"

namespace AudioCoPilot
{
    ChannelSelector::ChannelSelector()
    {
        _titleLabel = std::make_unique<juce::Label>("title", "CHANNEL CONFIGURATION");
        _titleLabel->setFont(juce::Font(juce::FontOptions("Helvetica Neue", 12.0f, juce::Font::bold)));
        _titleLabel->setColour(juce::Label::textColourId, Colours::TextPrimary);
        _titleLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*_titleLabel);
        
        _targetLabel = std::make_unique<juce::Label>("target", "Target:");
        _targetLabel->setFont(juce::Font(juce::FontOptions("Helvetica Neue", 11.0f, juce::Font::plain)));
        _targetLabel->setColour(juce::Label::textColourId, Colours::TextPrimary);
        addAndMakeVisible(*_targetLabel);
        
        // Inicializa selectors de canal
        for (int i = 0; i < 4; ++i)
        {
            _channelSelectors[i] = std::make_unique<juce::ComboBox>("Channel" + juce::String(i));
            _channelSelectors[i]->addItem("Channel " + juce::String(i + 1), i + 1);
            _channelSelectors[i]->setSelectedId(1, juce::dontSendNotification);
            _channelSelectors[i]->addListener(this);
            addAndMakeVisible(*_channelSelectors[i]);
        }
        
        // Inicializa maskers
        for (int i = 0; i < 3; ++i)
        {
            _maskerToggles[i] = std::make_unique<juce::ToggleButton>("Masker " + juce::String(i + 1));
            _maskerToggles[i]->setButtonText("Masker " + juce::String(i + 1));
            _maskerToggles[i]->addListener(this);
            _maskerToggles[i]->setToggleState(false, juce::dontSendNotification);
            addAndMakeVisible(*_maskerToggles[i]);
            
            _maskerLabels[i] = std::make_unique<juce::Label>("MaskerLabel" + juce::String(i), "Ch:");
            _maskerLabels[i]->setFont(juce::Font(juce::FontOptions("Helvetica Neue", 11.0f, juce::Font::plain)));
            _maskerLabels[i]->setColour(juce::Label::textColourId, Colours::TextSecondary);
            addAndMakeVisible(*_maskerLabels[i]);
            
            _maskers[i].channel = i + 1;
        }
        
        // NÃO chama listener durante inicialização
        _numChannels = 4;
        
        // Atualiza todos os comboboxes sem chamar listener
        for (int i = 0; i < 4; ++i)
        {
            _channelSelectors[i]->clear();
            for (int ch = 1; ch <= 4; ++ch)
            {
                _channelSelectors[i]->addItem("Channel " + juce::String(ch), ch);
            }
            _channelSelectors[i]->setSelectedId(1, juce::dontSendNotification);
        }
        
        // Atualiza canais dos maskers sem chamar listener
        for (int i = 0; i < 3; ++i)
        {
            _maskers[i].channel = _channelSelectors[i + 1]->getSelectedId();
        }
        _targetChannel = _channelSelectors[0]->getSelectedId() - 1;
    }
    
    void ChannelSelector::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(Colours::BackgroundDark);
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Borda
        g.setColour(Colours::Border);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }
    
    void ChannelSelector::resized()
    {
        auto bounds = getLocalBounds().reduced(10);
        
        // Título
        _titleLabel->setBounds(bounds.removeFromTop(25));
        
        // Target
        auto targetRow = bounds.removeFromTop(30);
        _targetLabel->setBounds(targetRow.removeFromLeft(60).reduced(2));
        _channelSelectors[0]->setBounds(targetRow.removeFromLeft(100).reduced(2));
        
        // Maskers
        for (int i = 0; i < 3; ++i)
        {
            auto maskerRow = bounds.removeFromTop(30);
            
            // Toggle
            _maskerToggles[i]->setBounds(maskerRow.removeFromLeft(100).reduced(2));
            
            // Label
            _maskerLabels[i]->setBounds(maskerRow.removeFromLeft(30).reduced(2));
            
            // Selector
            _channelSelectors[i + 1]->setBounds(maskerRow.removeFromLeft(100).reduced(2));
        }
    }
    
void ChannelSelector::buttonClicked(juce::Button* button)
{
    for (int i = 0; i < 3; ++i)
    {
        if (button == _maskerToggles[i].get())
        {
            _maskers[i].enabled = button->getToggleState();
            
            // Atualiza visibilidade do selector
            _channelSelectors[i + 1]->setEnabled(_maskers[i].enabled);
            _maskerLabels[i]->setEnabled(_maskers[i].enabled);
            
            if (_changeListener)
                _changeListener();
            
            break;
        }
    }
}

void ChannelSelector::comboBoxChanged(juce::ComboBox* comboBox)
{
    // Quando um combobox muda, atualiza a configuração
    updateChannelSelectors();
}
    
    void ChannelSelector::setNumChannels(int numChannels)
    {
        _numChannels = numChannels;
        
        // Atualiza todos os comboboxes
        for (int i = 0; i < 4; ++i)
        {
            _channelSelectors[i]->clear();
            for (int ch = 1; ch <= numChannels; ++ch)
            {
                _channelSelectors[i]->addItem("Channel " + juce::String(ch), ch);
            }
            
            // Mantém seleção atual se possível
            int currentId = _channelSelectors[i]->getSelectedId();
            if (currentId > 0 && currentId <= numChannels)
                _channelSelectors[i]->setSelectedId(currentId, juce::dontSendNotification);
            else
                _channelSelectors[i]->setSelectedId(1, juce::dontSendNotification);
        }
        
        updateChannelSelectors();
    }
    
    bool ChannelSelector::isMaskerEnabled(int maskerIndex) const
    {
        if (maskerIndex >= 0 && maskerIndex < 3)
            return _maskers[maskerIndex].enabled;
        return false;
    }
    
    int ChannelSelector::getMaskerChannel(int maskerIndex) const
    {
        if (maskerIndex >= 0 && maskerIndex < 3)
            return _maskers[maskerIndex].channel - 1; // Converte para 0-based
        return 0;
    }
    
    void ChannelSelector::setListener(std::function<void()> listener)
    {
        _changeListener = listener;
    }
    
    void ChannelSelector::updateChannelSelectors()
    {
        // Atualiza canais dos maskers baseado nos comboboxes
        for (int i = 0; i < 3; ++i)
        {
            _maskers[i].channel = _channelSelectors[i + 1]->getSelectedId();
        }
        
        // Target channel
        _targetChannel = _channelSelectors[0]->getSelectedId() - 1; // Converte para 0-based
        
        // Só chama listener se estiver configurado (não durante inicialização)
        if (_changeListener)
        {
            _changeListener();
        }
    }
}