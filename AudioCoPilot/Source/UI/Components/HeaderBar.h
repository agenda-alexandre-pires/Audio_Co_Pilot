#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "../../Core/DeviceManager.h"
#include "DeviceSelector.h"

class HeaderBar : public juce::Component,
                  public juce::Button::Listener
{
public:
    HeaderBar();
    ~HeaderBar() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setDeviceManager(DeviceManager* manager);
    
    // Callbacks para mudança de view mode
    std::function<void()> onMetersClicked;
    std::function<void()> onTransferFunctionClicked;
    std::function<void()> onFeedbackPredictionClicked;
    std::function<void()> onAntiMaskingClicked;
    
    // Atualiza qual botão está ativo
    void setActiveModule(const juce::String& moduleName);
    
    // Acesso ao DeviceSelector (para configurar callbacks)
    DeviceSelector* getDeviceSelector() { return _deviceSelector.get(); }

private:
    void buttonClicked(juce::Button* button) override;
    
    std::unique_ptr<juce::Label> _titleLabel;
    std::unique_ptr<DeviceSelector> _deviceSelector;
    std::unique_ptr<juce::TextButton> _metersButton;
    std::unique_ptr<juce::TextButton> _transferFunctionButton;
    std::unique_ptr<juce::TextButton> _feedbackPredictionButton;
    std::unique_ptr<juce::TextButton> _antiMaskingButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderBar)
};

