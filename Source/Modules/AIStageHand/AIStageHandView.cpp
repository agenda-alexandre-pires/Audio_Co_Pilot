#include "AIStageHandView.h"

namespace AudioCoPilot
{

AIStageHandView::AIStageHandView(AIStageHandController& c)
    : controller(c)
{
    setOpaque(true);

    alertBox.setReadOnly(true);
    alertBox.setMultiLine(true);
    alertBox.setScrollbarsShown(true);
    alertBox.setCaretVisible(false);
    using namespace AudioCoPilot::DesignSystem;
    alertBox.setColour(juce::TextEditor::backgroundColourId, Colours::getColour(Colours::Surface::Panel));
    alertBox.setColour(juce::TextEditor::outlineColourId, Colours::getColour(Colours::Border::Default));
    alertBox.setColour(juce::TextEditor::textColourId, Colours::getColour(Colours::Text::Primary));
    alertBox.setFont(Typography::bodySmall());
    addAndMakeVisible(alertBox);

    startTimerHz(20); // 50ms
}

AIStageHandView::~AIStageHandView()
{
    stopTimer();
}

void AIStageHandView::paint(juce::Graphics& g)
{
    using namespace AudioCoPilot::DesignSystem;
    g.fillAll(Colours::getColour(Colours::Surface::Background));

    const auto textColour = Colours::getColour(Colours::Text::Primary);
    g.setColour(textColour);
    g.setFont(Typography::headlineMedium());

    g.drawText("AI Stage Hand - Feedback Watch", getLocalBounds().removeFromTop(headerHeight),
               juce::Justification::centredLeft, false);

    // Meter grid area
    auto bounds = getLocalBounds();
    bounds.removeFromTop(headerHeight);
    auto metersArea = bounds.removeFromTop(bounds.getHeight() * 2 / 3).reduced(12, 8);

    const int total = (int) levels.size();
    const int rows = (total + maxPerRow - 1) / maxPerRow;
    const int meterHeight = juce::jmax(60, metersArea.getHeight() / juce::jmax(1, rows) - meterSpacing);

    const double nowMs = juce::Time::getMillisecondCounterHiRes();

    for (int idx = 0; idx < total; ++idx)
    {
        const int row = idx / maxPerRow;
        const int col = idx % maxPerRow;
        const int x = metersArea.getX() + col * (meterWidth + meterSpacing);
        const int y = metersArea.getY() + row * (meterHeight + meterSpacing);

        juce::Rectangle<int> meterRect(x, y, meterWidth, meterHeight);

        // Fundo
        using namespace AudioCoPilot::DesignSystem;
        g.setColour(Colours::getColour(Colours::Surface::MeterBackground));
        g.fillRect(meterRect);

        // Alerta pulsando em vermelho
        bool alertActive = false;
        if (idx < (int) alertsMs.size())
        {
            double age = nowMs - alertsMs[(size_t) idx];
            if (age >= 0.0 && age < 2000.0)
            {
                alertActive = true;
                float phase = (float) std::sin(age * 0.02);
                float alpha = juce::jlimit(0.3f, 0.9f, 0.6f + 0.3f * phase);
                g.setColour(Colours::getColour(Colours::Status::Alert).withAlpha(alpha));
                g.fillRect(meterRect);
            }
        }

        // Valor RMS -> dB
        float rms = juce::jlimit(0.0f, 1.0f, levels[(size_t) idx]);
        float db = 20.0f * std::log10(rms + 1.0e-6f);
        float norm = juce::jlimit(0.0f, 1.0f, 1.0f - ((-db) / 60.0f)); // -60dB..0dB

        auto bar = meterRect.withTrimmedTop((int) (meterRect.getHeight() * (1.0f - norm)));
        const auto meterColour = alertActive
            ? Colours::getColour(Colours::Status::Warning)
            : Colours::getColour(Colours::Status::Safe);
        g.setColour(meterColour);
        g.fillRect(bar);

        // Número do canal
        g.setColour(textColour);
        g.setFont(AudioCoPilot::DesignSystem::Typography::labelSmall());
        g.drawText(juce::String(idx + 1),
                   meterRect.withY(meterRect.getBottom() + 2).withHeight(16),
                   juce::Justification::centred, false);
    }
}

void AIStageHandView::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(headerHeight);
    auto metersArea = bounds.removeFromTop(bounds.getHeight() * 2 / 3);
    juce::ignoreUnused(metersArea);
    alertBox.setBounds(bounds.reduced(12, 8));
}

void AIStageHandView::timerCallback()
{
    levels = controller.getChannelRms();
    alertsMs = controller.getChannelAlertTimesMs();
    rebuildAlertText();
    repaint();
}

void AIStageHandView::rebuildAlertText()
{
    alertCache = controller.getAlertLog();
    juce::String text;
    for (int i = alertCache.size() - 1; i >= 0; --i)
    {
        text << alertCache[i] << "\n";
    }
    alertBox.setText(text.trimEnd(), juce::dontSendNotification);
}

} // namespace AudioCoPilot
