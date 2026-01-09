#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

class StatusBar : public juce::Component
{
public:
    enum class StatusType { Good, Warning, Error, Info };

    StatusBar();
    ~StatusBar() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setStatus(const juce::String& message, StatusType type = StatusType::Info);
    void setSampleRateAndBuffer(int sampleRate, int bufferSize);

private:
    juce::String _statusMessage = "Ready";
    StatusType _statusType = StatusType::Info;
    int _sampleRate = 0;
    int _bufferSize = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBar)
};

