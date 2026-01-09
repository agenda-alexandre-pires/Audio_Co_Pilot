#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

class TestMainComponent : public juce::Component
{
public:
    TestMainComponent()
    {
        std::cout << "[TestMainComponent] Constructor started" << std::endl;
        
        // Componente mínimo - apenas um label
        _testLabel = std::make_unique<juce::Label>("TestLabel", "Audio Co-Pilot (Test Mode)");
        _testLabel->setColour(juce::Label::textColourId, juce::Colours::white);
        _testLabel->setFont(juce::Font(24.0f, juce::Font::bold));
        _testLabel->setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(*_testLabel);
        
        std::cout << "[TestMainComponent] Constructor completed" << std::endl;
        
        setSize(800, 600);
    }
    
    ~TestMainComponent()
    {
        std::cout << "[TestMainComponent] Destructor" << std::endl;
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF1A1A1A)); // Background escuro
    }
    
    void resized() override
    {
        if (_testLabel)
        {
            _testLabel->setBounds(getLocalBounds());
        }
    }
    
private:
    std::unique_ptr<juce::Label> _testLabel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestMainComponent)
};