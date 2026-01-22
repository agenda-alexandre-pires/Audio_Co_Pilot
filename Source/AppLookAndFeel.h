#pragma once
#include "Theme.h"

class AppLookAndFeel : public juce::LookAndFeel_V4 {
public:
    AppLookAndFeel() {
        setColour (juce::ResizableWindow::backgroundColourId, Theme::background);
        setColour (juce::TextButton::buttonColourId, Theme::surface);
        setColour (juce::TextButton::textColourOffId, Theme::textMain);
        setColour (juce::ComboBox::backgroundColourId, Theme::background);
        setColour (juce::ComboBox::outlineColourId, Theme::grid);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&, bool isHighlighted, bool isDown) override {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (button.getToggleState() ? Theme::accent : Theme::surface);
        if (isHighlighted) g.setColour (g.findColour(juce::TextButton::buttonColourId).brighter(0.1f));
        g.fillRoundedRectangle (bounds, Theme::cornerRadius);
    }
    
    // Customização para ComboBoxes (Seletores de Device)
    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override {
        auto bounds = juce::Rectangle<int>(width, height).toFloat().reduced (0.5f);
        g.setColour (Theme::background);
        g.fillRoundedRectangle (bounds, Theme::cornerRadius);
        g.setColour (Theme::grid);
        g.drawRoundedRectangle (bounds, Theme::cornerRadius, 1.0f);
    }
};
