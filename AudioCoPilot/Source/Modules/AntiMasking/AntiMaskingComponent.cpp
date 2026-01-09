#include "AntiMaskingComponent.h"
#include "../../UI/Colours.h"

namespace AudioCoPilot
{
    AntiMaskingComponent::AntiMaskingComponent()
    {
        _titleLabel = std::make_unique<juce::Label>("title", "ANTI-MASKING ANALYSIS");
        _titleLabel->setFont(juce::Font(juce::FontOptions("Helvetica Neue", 16.0f, juce::Font::bold)));
        _titleLabel->setColour(juce::Label::textColourId, Colours::Accent);
        _titleLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*_titleLabel);
        
        _matrixDisplay = std::make_unique<MaskingMatrixDisplay>();
        addAndMakeVisible(*_matrixDisplay);
        
        _targetSpectrum = std::make_unique<BarkSpectrumDisplay>();
        _targetSpectrum->setChannelName("TARGET");
        _targetSpectrum->setChannelColour(Colours::Accent);
        addAndMakeVisible(*_targetSpectrum);
        
        _maskerSpectrum1 = std::make_unique<BarkSpectrumDisplay>();
        _maskerSpectrum1->setChannelName("MASKER 1");
        _maskerSpectrum1->setChannelColour(juce::Colour(0xFF00AAFF)); // Azul
        addAndMakeVisible(*_maskerSpectrum1);
        
        _maskerSpectrum2 = std::make_unique<BarkSpectrumDisplay>();
        _maskerSpectrum2->setChannelName("MASKER 2");
        _maskerSpectrum2->setChannelColour(juce::Colour(0xFFFF00AA)); // Magenta
        addAndMakeVisible(*_maskerSpectrum2);
        
        _maskerSpectrum3 = std::make_unique<BarkSpectrumDisplay>();
        _maskerSpectrum3->setChannelName("MASKER 3");
        _maskerSpectrum3->setChannelColour(juce::Colour(0xFFAAFF00)); // Verde-limão
        addAndMakeVisible(*_maskerSpectrum3);
        
        _channelSelector = std::make_unique<ChannelSelector>();
        addAndMakeVisible(*_channelSelector);
        
        _audibilityLabel = std::make_unique<juce::Label>("audibility", "Audibility: --");
        _audibilityLabel->setFont(juce::Font(juce::FontOptions("Helvetica Neue", 12.0f, juce::Font::plain)));
        _audibilityLabel->setColour(juce::Label::textColourId, Colours::TextPrimary);
        addAndMakeVisible(*_audibilityLabel);
        
        _criticalBandsLabel = std::make_unique<juce::Label>("critical", "Critical Bands: --");
        _criticalBandsLabel->setFont(juce::Font(juce::FontOptions("Helvetica Neue", 12.0f, juce::Font::plain)));
        _criticalBandsLabel->setColour(juce::Label::textColourId, Colours::TextPrimary);
        addAndMakeVisible(*_criticalBandsLabel);
        
        // Configura cores dos maskers no display
        _matrixDisplay->setMaskerColours(
            juce::Colour(0xFF00AAFF),
            juce::Colour(0xFFFF00AA),
            juce::Colour(0xFFAAFF00)
        );
        
        // Inicia timer para atualização
        startTimerHz(30); // 30 FPS
    }
    
    AntiMaskingComponent::~AntiMaskingComponent()
    {
        stopTimer();
    }
    
    void AntiMaskingComponent::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(Colours::Background);
        g.fillRect(bounds);
        
        // Borda
        g.setColour(Colours::Border);
        g.drawRect(bounds, 1.0f);
    }
    
    void AntiMaskingComponent::resized()
    {
        auto bounds = getLocalBounds().reduced(10);
        
        // Título
        _titleLabel->setBounds(bounds.removeFromTop(30));
        bounds.removeFromTop(10);
        
        // Primeira linha: Matrix Display (maior)
        auto topRow = bounds.removeFromTop(250);
        _matrixDisplay->setBounds(topRow);
        
        bounds.removeFromTop(10);
        
        // Segunda linha: Spectra
        auto middleRow = bounds.removeFromTop(120);
        int spectrumWidth = middleRow.getWidth() / 4;
        
        _targetSpectrum->setBounds(middleRow.removeFromLeft(spectrumWidth).reduced(5, 0));
        _maskerSpectrum1->setBounds(middleRow.removeFromLeft(spectrumWidth).reduced(5, 0));
        _maskerSpectrum2->setBounds(middleRow.removeFromLeft(spectrumWidth).reduced(5, 0));
        _maskerSpectrum3->setBounds(middleRow.removeFromLeft(spectrumWidth).reduced(5, 0));
        
        bounds.removeFromTop(10);
        
        // Terceira linha: Channel Selector e métricas
        auto bottomRow = bounds.removeFromTop(150);
        
        // Channel selector à esquerda
        _channelSelector->setBounds(bottomRow.removeFromLeft(250).reduced(5));
        
        // Métricas à direita
        auto metricsBounds = bottomRow.reduced(20, 20);
        _audibilityLabel->setBounds(metricsBounds.removeFromTop(25));
        _criticalBandsLabel->setBounds(metricsBounds.removeFromTop(25));
    }
    
    void AntiMaskingComponent::timerCallback()
    {
        updateDisplay();
    }
    
    void AntiMaskingComponent::setProcessor(AntiMaskingProcessor* processor)
    {
        _processor = processor;
        
        if (_processor)
        {
            // Configura o channel selector para notificar o processador
            _channelSelector->setListener([this]() {
                if (_processor)
                {
                    _processor->setTargetChannel(_channelSelector->getTargetChannel());
                    
                    for (int i = 0; i < 3; ++i)
                    {
                        _processor->setMaskerChannel(
                            i,
                            _channelSelector->getMaskerChannel(i),
                            _channelSelector->isMaskerEnabled(i)
                        );
                    }
                }
            });
        }
    }
    
    void AntiMaskingComponent::updateDisplay()
    {
        if (!_processor || !_processor->isPrepared())
            return;
        
        try {
            // Atualiza espectros
            _targetSpectrum->setSpectrum(_processor->getChannelBarkSpectrum(0));
            _maskerSpectrum1->setSpectrum(_processor->getChannelBarkSpectrum(1));
            _maskerSpectrum2->setSpectrum(_processor->getChannelBarkSpectrum(2));
            _maskerSpectrum3->setSpectrum(_processor->getChannelBarkSpectrum(3));
            
            // Atualiza matriz de mascaramento
            const auto& result = _processor->getCurrentResult();
            _matrixDisplay->setMaskingResult(result);
            
            // Atualiza métricas
            _audibilityLabel->setText("Audibility: " + 
                juce::String(result.overallAudibility * 100.0f, 1) + "%",
                juce::dontSendNotification);
            
            _criticalBandsLabel->setText("Critical Bands: " + 
                juce::String(static_cast<int>(result.criticalBandCount)) + "/24",
                juce::dontSendNotification);
            
            // Atualiza cores baseadas na audibilidade
            if (result.overallAudibility > 0.7f)
                _audibilityLabel->setColour(juce::Label::textColourId, Colours::Safe);
            else if (result.overallAudibility > 0.4f)
                _audibilityLabel->setColour(juce::Label::textColourId, Colours::Warning);
            else
                _audibilityLabel->setColour(juce::Label::textColourId, Colours::Alert);
        }
        catch (const std::exception& e)
        {
            // Erro silencioso - não pode bloquear o timer
            static int errorCount = 0;
            if (errorCount++ < 3)
            {
                std::cerr << "[AntiMaskingComponent] EXCEPTION in updateDisplay: " << e.what() << std::endl;
            }
        }
        catch (...)
        {
            // Erro silencioso
        }
    }
}