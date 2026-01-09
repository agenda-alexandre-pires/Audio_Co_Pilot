#include "HeaderBar.h"
#include "../Colours.h"
#include "../../Core/DebugLogger.h"

HeaderBar::HeaderBar()
{
    _titleLabel = std::make_unique<juce::Label>("title", "AUDIO CO-PILOT");
    _titleLabel->setFont(juce::Font(juce::FontOptions("Helvetica Neue", 18.0f, juce::Font::bold)));
    _titleLabel->setColour(juce::Label::textColourId, Colours::Accent);
    _titleLabel->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*_titleLabel);

    _deviceSelector = std::make_unique<DeviceSelector>();
    addAndMakeVisible(*_deviceSelector);
    
    // Botões para cada módulo
    _metersButton = std::make_unique<juce::TextButton>("Meters");
    _metersButton->setButtonText("Meters");
    _metersButton->addListener(this);
    _metersButton->setColour(juce::TextButton::buttonColourId, Colours::Accent.withAlpha(0.3f));
    _metersButton->setColour(juce::TextButton::textColourOffId, Colours::TextPrimary);
    addAndMakeVisible(*_metersButton);
    
    _transferFunctionButton = std::make_unique<juce::TextButton>("TransferFunction");
    _transferFunctionButton->setButtonText("Transfer Function");
    _transferFunctionButton->addListener(this);
    _transferFunctionButton->setColour(juce::TextButton::buttonColourId, Colours::Accent.withAlpha(0.3f));
    _transferFunctionButton->setColour(juce::TextButton::textColourOffId, Colours::TextPrimary);
    addAndMakeVisible(*_transferFunctionButton);
    
    _feedbackPredictionButton = std::make_unique<juce::TextButton>("FeedbackPrediction");
    _feedbackPredictionButton->setButtonText("Feedback Prediction");
    _feedbackPredictionButton->addListener(this);
    _feedbackPredictionButton->setColour(juce::TextButton::buttonColourId, Colours::Accent.withAlpha(0.3f));
    _feedbackPredictionButton->setColour(juce::TextButton::textColourOffId, Colours::TextPrimary);
    addAndMakeVisible(*_feedbackPredictionButton);
    
    _antiMaskingButton = std::make_unique<juce::TextButton>("AntiMasking");
    _antiMaskingButton->setButtonText("Anti-Masking");
    _antiMaskingButton->addListener(this);
    _antiMaskingButton->setColour(juce::TextButton::buttonColourId, Colours::Accent.withAlpha(0.3f));
    _antiMaskingButton->setColour(juce::TextButton::textColourOffId, Colours::TextPrimary);
    addAndMakeVisible(*_antiMaskingButton);
    
    // Inicialmente Meters está ativo
    setActiveModule("Meters");
}

void HeaderBar::paint(juce::Graphics& g)
{
    g.fillAll(Colours::BackgroundLight);
    
    // Linha separadora embaixo
    g.setColour(Colours::Border);
    g.drawLine(0.0f, static_cast<float>(getHeight()) - 1.0f,
               static_cast<float>(getWidth()), static_cast<float>(getHeight()) - 1.0f, 1.0f);
}

void HeaderBar::resized()
{
    auto bounds = getLocalBounds().reduced(10, 5);
    
    // Título à esquerda
    _titleLabel->setBounds(bounds.removeFromLeft(200));
    bounds.removeFromLeft(10);
    
    // Botões dos módulos no meio
    int buttonWidth = 140;
    _metersButton->setBounds(bounds.removeFromLeft(buttonWidth).withSizeKeepingCentre(buttonWidth, 30));
    bounds.removeFromLeft(5);
    _transferFunctionButton->setBounds(bounds.removeFromLeft(buttonWidth + 20).withSizeKeepingCentre(buttonWidth + 20, 30));
    bounds.removeFromLeft(5);
    _feedbackPredictionButton->setBounds(bounds.removeFromLeft(buttonWidth + 20).withSizeKeepingCentre(buttonWidth + 20, 30));
    bounds.removeFromLeft(5);
    _antiMaskingButton->setBounds(bounds.removeFromLeft(buttonWidth).withSizeKeepingCentre(buttonWidth, 30));
    bounds.removeFromLeft(10);
    
    // Device selector à direita
    bounds.removeFromRight(10);
    _deviceSelector->setBounds(bounds.removeFromRight(300).withSizeKeepingCentre(300, 30));
}

void HeaderBar::buttonClicked(juce::Button* button)
{
    DEBUG_LOG("HeaderBar::buttonClicked - Button pressed");
    
    try {
        if (button == _metersButton.get())
        {
            DEBUG_LOG("Meters button clicked");
            if (onMetersClicked) {
                DEBUG_LOG("Calling onMetersClicked callback");
                onMetersClicked();
            } else {
                DEBUG_LOG("WARNING: onMetersClicked callback is null!");
            }
        }
        else if (button == _transferFunctionButton.get())
        {
            DEBUG_LOG("TransferFunction button clicked");
            if (onTransferFunctionClicked) {
                DEBUG_LOG("Calling onTransferFunctionClicked callback");
                onTransferFunctionClicked();
            } else {
                DEBUG_LOG("WARNING: onTransferFunctionClicked callback is null!");
            }
        }
        else if (button == _feedbackPredictionButton.get())
        {
            DEBUG_LOG("FeedbackPrediction button clicked");
            if (onFeedbackPredictionClicked) {
                DEBUG_LOG("Calling onFeedbackPredictionClicked callback");
                onFeedbackPredictionClicked();
            } else {
                DEBUG_LOG("WARNING: onFeedbackPredictionClicked callback is null!");
            }
        }
        else if (button == _antiMaskingButton.get())
        {
            DEBUG_LOG("AntiMasking button clicked");
            if (onAntiMaskingClicked) {
                DEBUG_LOG("Calling onAntiMaskingClicked callback");
                onAntiMaskingClicked();
            } else {
                DEBUG_LOG("WARNING: onAntiMaskingClicked callback is null!");
            }
        }
        else
        {
            DEBUG_LOG("Unknown button clicked");
        }
        
        DEBUG_LOG("HeaderBar::buttonClicked completed");
    }
    catch (const std::exception& e) {
        DEBUG_LOG("EXCEPTION in HeaderBar::buttonClicked: " + juce::String(e.what()));
        throw;
    }
    catch (...) {
        DEBUG_LOG("UNKNOWN EXCEPTION in HeaderBar::buttonClicked");
        throw;
    }
}

void HeaderBar::setDeviceManager(DeviceManager* manager)
{
    _deviceSelector->setDeviceManager(manager);
}

void HeaderBar::setActiveModule(const juce::String& moduleName)
{
    // Reseta todos os botões
    _metersButton->setColour(juce::TextButton::buttonColourId, Colours::Accent.withAlpha(0.3f));
    _transferFunctionButton->setColour(juce::TextButton::buttonColourId, Colours::Accent.withAlpha(0.3f));
    _feedbackPredictionButton->setColour(juce::TextButton::buttonColourId, Colours::Accent.withAlpha(0.3f));
    _antiMaskingButton->setColour(juce::TextButton::buttonColourId, Colours::Accent.withAlpha(0.3f));
    
    // Ativa o botão correspondente
    if (moduleName == "Meters")
    {
        _metersButton->setColour(juce::TextButton::buttonColourId, Colours::Accent);
    }
    else if (moduleName == "TransferFunction")
    {
        _transferFunctionButton->setColour(juce::TextButton::buttonColourId, Colours::Accent);
    }
    else if (moduleName == "FeedbackPrediction")
    {
        _feedbackPredictionButton->setColour(juce::TextButton::buttonColourId, Colours::Accent);
    }
    else if (moduleName == "AntiMasking")
    {
        _antiMaskingButton->setColour(juce::TextButton::buttonColourId, Colours::Accent);
    }
    
    repaint();
}

