#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "FeedbackPredictionProcessor.h"
#include "TrafficLightComponent.h"
#include <vector>

namespace AudioCoPilot {

class FeedbackPredictionComponent : public juce::Component,
                                     public juce::Timer {
public:
    FeedbackPredictionComponent(FeedbackPredictionProcessor& processor);
    ~FeedbackPredictionComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
private:
    FeedbackPredictionProcessor& _processor;
    
    // UI Elements
    TrafficLightComponent _trafficLight;
    juce::TextButton _enableButton { "Enable" };
    juce::TextButton _learnButton { "Learn" };
    juce::TextButton _clearButton { "Clear" };
    juce::Label _titleLabel { {}, "FEEDBACK PREDICTION" };
    
    // Lista de frequências em risco
    juce::ListBox _frequencyList;
    
    class FrequencyListModel : public juce::ListBoxModel {
    public:
        std::vector<FeedbackCandidate> candidates;
        
        int getNumRows() override { return static_cast<int>(candidates.size()); }
        void paintListBoxItem(int row, juce::Graphics& g, 
                             int width, int height, bool selected) override;
    };
    FrequencyListModel _listModel;
    
    void updateUI();
    void onEnableClicked();
    void onLearnClicked();
    void onClearClicked();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FeedbackPredictionComponent)
};

} // namespace AudioCoPilot

