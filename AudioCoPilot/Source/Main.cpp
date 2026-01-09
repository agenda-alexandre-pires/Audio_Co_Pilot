/*
    Audio Co-Pilot
    Copyright (c) 2024 Emerson Porfa Audio
    www.emersonporfaaudio.com.br
*/

#include <juce_gui_extra/juce_gui_extra.h>
#include "MainComponent.h"
#include "TestMainComponent.h"
#include "UI/LookAndFeel/BrutalistLookAndFeel.h"

class AudioCoPilotApplication : public juce::JUCEApplication
{
public:
    AudioCoPilotApplication() {}

    const juce::String getApplicationName() override       { return "Audio Co-Pilot"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override             { return false; }

    void initialise(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
        
        // Configura Look and Feel global
        _lookAndFeel = std::make_unique<BrutalistLookAndFeel>();
        juce::LookAndFeel::setDefaultLookAndFeel(_lookAndFeel.get());
        
        _mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
        _mainWindow = nullptr;
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
        _lookAndFeel = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name)
            : DocumentWindow(name,
                             juce::Colour(0xFF1A1A1A),  // Background Brutalist
                             DocumentWindow::allButtons)
        {
            std::cout << "[MainWindow] Constructor started" << std::endl;
            
            setUsingNativeTitleBar(true);
            
            // Usar MainComponent com carregamento gradual
            std::cout << "[MainWindow] Creating MainComponent (gradual loading)..." << std::endl;
            try {
                _mainComponent = std::make_unique<MainComponent>();
                setContentOwned(_mainComponent.release(), true);
                std::cout << "[MainWindow] MainComponent created successfully" << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "[MainWindow] ERROR creating MainComponent: " << e.what() << std::endl;
                // Fallback absoluto
                setContentOwned(new juce::Label("ErrorLabel", "Failed to load application: " + juce::String(e.what())), true);
            }
            catch (...) {
                std::cerr << "[MainWindow] UNKNOWN ERROR creating MainComponent" << std::endl;
                setContentOwned(new juce::Label("ErrorLabel", "Unknown error loading application"), true);
            }

            // Fullscreen resizable
            setResizable(true, true);
            setResizeLimits(1024, 600, 4096, 2160);
            
            // Centraliza e maximiza
            centreWithSize(getWidth(), getHeight());
            
            // Tenta pegar tamanho da tela e usar 80%
            if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
            {
                auto area = display->userArea;
                setSize(static_cast<int>(area.getWidth() * 0.85f),
                        static_cast<int>(area.getHeight() * 0.85f));
                centreWithSize(getWidth(), getHeight());
            }

            std::cout << "[MainWindow] Making window visible..." << std::endl;
            setVisible(true);
            std::cout << "[MainWindow] Window should now be visible" << std::endl;
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        std::unique_ptr<MainComponent> _mainComponent;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> _mainWindow;
    std::unique_ptr<BrutalistLookAndFeel> _lookAndFeel;
};

START_JUCE_APPLICATION(AudioCoPilotApplication)

