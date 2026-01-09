#include "BrutalistLookAndFeel.h"
#include "../Colours.h"

BrutalistLookAndFeel::BrutalistLookAndFeel()
{
    // Cores base do scheme
    setColour(juce::ResizableWindow::backgroundColourId, Colours::Background);
    setColour(juce::DocumentWindow::backgroundColourId, Colours::Background);
    
    // Labels
    setColour(juce::Label::textColourId, Colours::TextPrimary);
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    
    // Buttons
    setColour(juce::TextButton::buttonColourId, Colours::BackgroundLight);
    setColour(juce::TextButton::buttonOnColourId, Colours::Accent);
    setColour(juce::TextButton::textColourOffId, Colours::TextPrimary);
    setColour(juce::TextButton::textColourOnId, Colours::Background);
    
    // ComboBox
    setColour(juce::ComboBox::backgroundColourId, Colours::BackgroundLight);
    setColour(juce::ComboBox::textColourId, Colours::TextPrimary);
    setColour(juce::ComboBox::outlineColourId, Colours::Border);
    setColour(juce::ComboBox::arrowColourId, Colours::TextSecondary);
    
    // PopupMenu
    setColour(juce::PopupMenu::backgroundColourId, Colours::BackgroundLight);
    setColour(juce::PopupMenu::textColourId, Colours::TextPrimary);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Colours::Accent);
    setColour(juce::PopupMenu::highlightedTextColourId, Colours::Background);
    
    // ScrollBar
    setColour(juce::ScrollBar::thumbColourId, Colours::BackgroundLight);
    setColour(juce::ScrollBar::trackColourId, Colours::BackgroundDark);
}

juce::Font BrutalistLookAndFeel::getLabelFont(juce::Label& label)
{
    juce::ignoreUnused(label);
    return juce::Font(juce::FontOptions("Helvetica Neue", 14.0f, juce::Font::plain));
}

juce::Font BrutalistLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    juce::ignoreUnused(box);
    return juce::Font(juce::FontOptions("Helvetica Neue", 14.0f, juce::Font::plain));
}

juce::Font BrutalistLookAndFeel::getTextButtonFont(juce::TextButton& button, int buttonHeight)
{
    juce::ignoreUnused(button, buttonHeight);
    return juce::Font(juce::FontOptions("Helvetica Neue", 13.0f, juce::Font::bold));
}

void BrutalistLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                                 juce::Button& button,
                                                 const juce::Colour& backgroundColour,
                                                 bool shouldDrawButtonAsHighlighted,
                                                 bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    auto baseColour = backgroundColour;

    if (shouldDrawButtonAsDown)
        baseColour = baseColour.darker(0.3f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter(0.1f);

    // Fundo
    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, 3.0f);

    // Borda
    g.setColour(Colours::Border);
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
}

void BrutalistLookAndFeel::drawComboBox(juce::Graphics& g,
                                         int width, int height,
                                         bool isButtonDown,
                                         int buttonX, int buttonY,
                                         int buttonW, int buttonH,
                                         juce::ComboBox& box)
{
    juce::ignoreUnused(buttonX, buttonY, buttonW, buttonH);
    
    auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height));
    auto cornerSize = 3.0f;

    // Fundo
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds.reduced(0.5f), cornerSize);

    // Borda
    g.setColour(isButtonDown ? Colours::Accent : Colours::Border);
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);

    // Seta
    auto arrowZone = juce::Rectangle<float>(static_cast<float>(width - 25), 0.0f, 20.0f, static_cast<float>(height));
    juce::Path arrow;
    arrow.addTriangle(arrowZone.getCentreX() - 4.0f, arrowZone.getCentreY() - 2.0f,
                      arrowZone.getCentreX() + 4.0f, arrowZone.getCentreY() - 2.0f,
                      arrowZone.getCentreX(), arrowZone.getCentreY() + 4.0f);
    
    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.fillPath(arrow);
}

void BrutalistLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                             int x, int y, int width, int height,
                                             float sliderPos,
                                             float minSliderPos,
                                             float maxSliderPos,
                                             const juce::Slider::SliderStyle style,
                                             juce::Slider& slider)
{
    juce::ignoreUnused(minSliderPos, maxSliderPos, style);
    
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    auto trackWidth = 4.0f;

    // Track background
    auto trackBounds = bounds.withSizeKeepingCentre(bounds.getWidth(), trackWidth);
    g.setColour(Colours::BackgroundDark);
    g.fillRoundedRectangle(trackBounds, 2.0f);

    // Filled portion
    auto fillWidth = sliderPos - static_cast<float>(x);
    g.setColour(Colours::Accent);
    g.fillRoundedRectangle(trackBounds.withWidth(fillWidth), 2.0f);

    // Thumb
    auto thumbWidth = 12.0f;
    auto thumbBounds = juce::Rectangle<float>(sliderPos - thumbWidth / 2.0f,
                                               bounds.getCentreY() - thumbWidth / 2.0f,
                                               thumbWidth, thumbWidth);
    
    g.setColour(slider.isMouseOver() ? Colours::AccentBright : Colours::Accent);
    g.fillEllipse(thumbBounds);
}

