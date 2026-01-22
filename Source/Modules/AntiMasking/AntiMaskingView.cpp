#include "AntiMaskingView.h"
#include "../../Localization/LocalizedStrings.h"

namespace AudioCoPilot
{
AntiMaskingView::AntiMaskingView (AntiMaskingController& c)
    : controller (c)
{
    auto& strings = LocalizedStrings::getInstance();

    // Title - visual melhorado (maior, mais destacado)
    title.setColour (juce::Label::textColourId, juce::Colours::white);
    title.setJustificationType (juce::Justification::centredLeft);
    title.setFont (juce::Font (16.0f, juce::Font::bold));
    addAndMakeVisible (title);

    title.setText (strings.getAntiMaskingTitle(), juce::dontSendNotification);

    // Target label - visual melhorado (como no design)
    targetLabel.setText (strings.getAntiMaskingTargetLabel(), juce::dontSendNotification);
    targetLabel.setColour (juce::Label::textColourId, juce::Colour (0xff3b82f6)); // Azul como no design
    targetLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    addAndMakeVisible (targetLabel);

    targetCombo.addListener (this);
    addAndMakeVisible (targetCombo);

    for (int i = 0; i < 3; ++i)
    {
        maskerEnable[(size_t) i].setButtonText (strings.getAntiMaskingEnable());
        maskerEnable[(size_t) i].addListener (this);
        addAndMakeVisible (maskerEnable[(size_t) i]);

        maskerLabel[(size_t) i].setText (strings.getAntiMaskingMaskerLabel (i + 1), juce::dontSendNotification);
        maskerLabel[(size_t) i].setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (maskerLabel[(size_t) i]);

        maskerCombo[(size_t) i].addListener (this);
        addAndMakeVisible (maskerCombo[(size_t) i]);
    }

    addAndMakeVisible (matrix);

    targetSpectrum.setNameAndColour (strings.getAntiMaskingTargetSpectrumTitle(), juce::Colours::orange);
    addAndMakeVisible (targetSpectrum);

    maskerSpectra[0].setNameAndColour (strings.getAntiMaskingMaskerSpectrumTitle (1), juce::Colours::deepskyblue);
    maskerSpectra[1].setNameAndColour (strings.getAntiMaskingMaskerSpectrumTitle (2), juce::Colours::magenta);
    maskerSpectra[2].setNameAndColour (strings.getAntiMaskingMaskerSpectrumTitle (3), juce::Colours::greenyellow);
    for (auto& s : maskerSpectra) addAndMakeVisible (s);

    suggestionsLabel.setText (strings.getAntiMaskingSuggestionsTitle(), juce::dontSendNotification);
    suggestionsLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (suggestionsLabel);

    suggestionsBox.setMultiLine (true);
    suggestionsBox.setReadOnly (true);
    suggestionsBox.setScrollbarsShown (true);
    suggestionsBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff1a1a1a));
    suggestionsBox.setColour (juce::TextEditor::textColourId, juce::Colours::white);
    suggestionsBox.setColour (juce::TextEditor::outlineColourId, juce::Colours::black.withAlpha (0.6f));
    addAndMakeVisible (suggestionsBox);

    controller.addChangeListener (this);
    rebuildChannelLists();

    strings.addChangeListener (this);
    // Timer will start when view becomes visible
}

void AntiMaskingView::visibilityChanged()
{
    if (isVisible())
    {
        if (! isTimerRunning())
            startTimerHz (20);
    }
    else
    {
        if (isTimerRunning())
            stopTimer();
    }
}

AntiMaskingView::~AntiMaskingView()
{
    stopTimer();
    controller.removeChangeListener (this);
    LocalizedStrings::getInstance().removeChangeListener (this);
}

void AntiMaskingView::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff121212));
}

void AntiMaskingView::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    auto header = bounds.removeFromTop (34);
    title.setBounds (header.removeFromLeft (400));

    auto controls = bounds.removeFromTop (70);
    auto row1 = controls.removeFromTop (26);

    targetLabel.setBounds (row1.removeFromLeft (70));
    targetCombo.setBounds (row1.removeFromLeft (180));

    controls.removeFromTop (6);
    auto row2 = controls.removeFromTop (26);

    for (int i = 0; i < 3; ++i)
    {
        auto block = row2.removeFromLeft (250);
        maskerEnable[(size_t) i].setBounds (block.removeFromLeft (70));
        maskerLabel[(size_t) i].setBounds (block.removeFromLeft (85));
        maskerCombo[(size_t) i].setBounds (block.removeFromLeft (95));
        row2.removeFromLeft (10);
    }

    // Layout:
    // Top: matrix (60%)
    // Bottom: spectra row (left) + suggestions (right)
    auto top = bounds.removeFromTop ((int) (bounds.getHeight() * 0.60f));
    matrix.setBounds (top);

    auto bottom = bounds;
    auto left = bottom.removeFromLeft ((int) (bottom.getWidth() * 0.58f));
    auto right = bottom;

    const int specH = left.getHeight() / 4;
    targetSpectrum.setBounds (left.removeFromTop (specH).reduced (0, 2));
    for (int i = 0; i < 3; ++i)
        maskerSpectra[(size_t) i].setBounds (left.removeFromTop (specH).reduced (0, 2));

    suggestionsLabel.setBounds (right.removeFromTop (18));
    suggestionsBox.setBounds (right.reduced (0, 2));
}

void AntiMaskingView::rebuildChannelLists()
{
    const int avail = controller.getAvailableInputChannels();

    targetCombo.clear();
    for (auto& cb : maskerCombo) cb.clear();

    for (int ch = 0; ch < avail; ++ch)
    {
        const auto name = "Ch " + juce::String (ch + 1);
        targetCombo.addItem (name, ch + 1);
        for (auto& cb : maskerCombo)
            cb.addItem (name, ch + 1);
    }

    targetCombo.setSelectedId (controller.getTargetChannel() + 1, juce::dontSendNotification);

    for (int i = 0; i < 3; ++i)
    {
        maskerCombo[(size_t) i].setSelectedId (controller.getMaskerChannel (i) + 1, juce::dontSendNotification);
        maskerEnable[(size_t) i].setToggleState (controller.isMaskerEnabled (i), juce::dontSendNotification);
    }
}

void AntiMaskingView::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &targetCombo)
    {
        controller.setTargetChannel (targetCombo.getSelectedId() - 1);
        return;
    }

    for (int i = 0; i < 3; ++i)
    {
        if (comboBoxThatHasChanged == &maskerCombo[(size_t) i])
        {
            controller.setMaskerChannel (i,
                                        maskerCombo[(size_t) i].getSelectedId() - 1,
                                        maskerEnable[(size_t) i].getToggleState());
            return;
        }
    }
}

void AntiMaskingView::buttonClicked (juce::Button* button)
{
    for (int i = 0; i < 3; ++i)
    {
        if (button == &maskerEnable[(size_t) i])
        {
            controller.setMaskerChannel (i,
                                        maskerCombo[(size_t) i].getSelectedId() - 1,
                                        maskerEnable[(size_t) i].getToggleState());
            return;
        }
    }
}

void AntiMaskingView::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source == &controller)
        rebuildChannelLists();
    else if (source == &LocalizedStrings::getInstance())
    {
        // refresh texts when language changes
        auto& strings = LocalizedStrings::getInstance();
        title.setText (strings.getAntiMaskingTitle(), juce::dontSendNotification);
        targetLabel.setText (strings.getAntiMaskingTargetLabel(), juce::dontSendNotification);
        for (int i = 0; i < 3; ++i)
        {
            maskerEnable[(size_t) i].setButtonText (strings.getAntiMaskingEnable());
            maskerLabel[(size_t) i].setText (strings.getAntiMaskingMaskerLabel (i + 1), juce::dontSendNotification);
            maskerSpectra[(size_t) i].setNameAndColour (strings.getAntiMaskingMaskerSpectrumTitle (i + 1),
                                                       maskerSpectra[(size_t) i].getColour());
        }
        targetSpectrum.setNameAndColour (strings.getAntiMaskingTargetSpectrumTitle(), juce::Colours::orange);
        suggestionsLabel.setText (strings.getAntiMaskingSuggestionsTitle(), juce::dontSendNotification);
    }
}

void AntiMaskingView::timerCallback()
{
    if (! isVisible())
    {
        stopTimer();
        return;
    }
    
    const auto& r = controller.getAveragedResult();
    matrix.setResult (r);

    // Spectra snapshot (target + 3 maskers as selected streams)
    const auto spectra = controller.getLatestSpectraDb();
    targetSpectrum.setSpectrumDb (spectra[0]);
    for (int i = 0; i < 3; ++i)
        maskerSpectra[(size_t) i].setSpectrumDb (spectra[(size_t) (i + 1)]);

    // Suggestions (IA heurística)
    auto& strings = LocalizedStrings::getInstance();
    auto sugg = AISuggestionsEngine::generate (r, strings);
    juce::String text;
    for (const auto& s : sugg)
    {
        text += "- " + s.title + "\n";
        text += "  " + s.details + "\n\n";
    }
    suggestionsBox.setText (text, false);
}
}

