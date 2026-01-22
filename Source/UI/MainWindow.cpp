#include "MainWindow.h"
#include "TransferFunction/TransferFunctionView.h"
#include "../Core/TransferFunction/TFController.h"
#include "../Modules/AntiMasking/AntiMaskingController.h"
#include "../Modules/AntiMasking/AntiMaskingView.h"
#include "../Modules/RTA/RTAController.h"
#include "../Modules/RTA/RTAView.h"
#include "../Modules/AIStageHand/AIStageHandController.h"
#include "../Modules/AIStageHand/AIStageHandView.h"
#include "../Localization/LocalizedStrings.h"

MainContentComponent::MainContentComponent()
{
    auto& strings = LocalizedStrings::getInstance();

    // Brand logo (loaded from app Resources at runtime) - positioned on the right
    brandLogo.setInterceptsMouseClicks (false, false);
    brandLogo.setImagePlacement (juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    addAndMakeVisible (brandLogo);

    footer.setJustificationType (juce::Justification::centred);
    footer.setColour (juce::Label::textColourId, AudioCoPilot::DesignSystem::Colours::getColour(AudioCoPilot::DesignSystem::Colours::Text::Primary).withAlpha (0.7f));
    footer.setText (strings.getBrandLine(), juce::dontSendNotification);
    addAndMakeVisible (footer);

    strings.addChangeListener (this);

    // Load logo once on the message thread (robust path resolution)
    auto tryLoad = [] (const juce::File& f) -> juce::Image
    {
        if (! f.existsAsFile()) return {};
        auto img = juce::ImageFileFormat::loadFrom (f);
        return img.isValid() ? img : juce::Image {};
    };

    const auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
    const auto app = juce::File::getSpecialLocation (juce::File::currentApplicationFile);

    // Typical: .../Audio Co-Pilot.app/Contents/MacOS/Audio Co-Pilot
    const auto resFromExe = exe.getParentDirectory().getParentDirectory().getChildFile ("Resources");
    const auto resFromApp = app.isDirectory() && app.hasFileExtension (".app")
                                ? app.getChildFile ("Contents").getChildFile ("Resources")
                                : app.getParentDirectory().getParentDirectory().getChildFile ("Resources");

    const auto logo1 = resFromExe.getChildFile ("AudioCoPilotLogo.png");
    const auto logo2 = resFromApp.getChildFile ("AudioCoPilotLogo.png");

    auto img = tryLoad (logo1);
    if (! img.isValid())
        img = tryLoad (logo2);

    if (img.isValid())
        brandLogo.setImage (img);
}

MainContentComponent::~MainContentComponent()
{
    LocalizedStrings::getInstance().removeChangeListener (this);
}

void MainContentComponent::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);
}

void MainContentComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source == &LocalizedStrings::getInstance())
        footer.setText (LocalizedStrings::getInstance().getBrandLine(), juce::dontSendNotification);
}

void MainContentComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Menu bar (if not native)
    #if !JUCE_MAC
    if (menuBar != nullptr)
        menuBar->setBounds(bounds.removeFromTop(25));
    #endif
    
    // Footer always visible
    auto footerArea = bounds.removeFromBottom (footerHeight);
    footer.setBounds (footerArea);

    // Brand header always visible (logo on the right, larger)
    auto headerArea = bounds.removeFromTop (brandHeaderHeight);
    const int logoSize = 75; // Larger logo
    const int logoPadding = 15;
    
    // Logo on the right side
    auto logoArea = headerArea.removeFromRight (logoSize + logoPadding * 2);
    brandLogo.setBounds (logoArea.reduced (logoPadding, (headerArea.getHeight() - logoSize) / 2).withSizeKeepingCentre (logoSize, logoSize));

    // Check if a module view is active (check all children)
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        if (dynamic_cast<TransferFunctionView*>(getChildComponent(i)) != nullptr
            || dynamic_cast<AudioCoPilot::AntiMaskingView*>(getChildComponent(i)) != nullptr
            || dynamic_cast<AudioCoPilot::RTAView*>(getChildComponent(i)) != nullptr
            || dynamic_cast<AudioCoPilot::AIStageHandView*>(getChildComponent(i)) != nullptr)
        {
            // Module is active - use all remaining space (between header and footer)
            getChildComponent(i)->setBounds(bounds);
            return;
        }
    }
    
    // Top bar: Device selectors (Input and Output separate)
    if (deviceSelector != nullptr)
    {
        auto topBar = bounds.removeFromTop(topBarHeight);
        deviceSelector->setBounds(topBar.reduced(10, 5));
    }
    
    // Meter area - side by side (input left 50%, output right 50%) with scroll
    if (inputViewport != nullptr && outputViewport != nullptr && 
        inputMeters != nullptr && outputMeters != nullptr)
    {
        // Use ALL remaining space below device selector for meters
        auto meterArea = bounds;
        const int halfWidth = meterArea.getWidth() / 2;
        
        // Calculate required height for both meters based on actual channel count
        int inputHeight = inputMeters->getRequiredHeight();
        int outputHeight = outputMeters->getRequiredHeight();
        
        // Ensure minimum height to show at least some meters
        if (inputHeight < 100) inputHeight = 100;
        if (outputHeight < 100) outputHeight = 100;
        
        // Set meter component sizes - use full viewport width, calculated height
        int inputViewportWidth = halfWidth - 10;
        int outputViewportWidth = meterArea.getWidth() - halfWidth - 10;
        
        // CRITICAL: Always resize meters to fit all channels of selected device
        // This ensures all channels (1-64) are visible with scroll if needed
        inputMeters->setSize(inputViewportWidth, inputHeight);
        outputMeters->setSize(outputViewportWidth, outputHeight);
        
        // Force viewports to update their scrollable area
        inputViewport->setViewPosition(0, 0);
        outputViewport->setViewPosition(0, 0);
        
        // Input viewport on LEFT HALF (0% to 50%)
        auto inputArea = meterArea.removeFromLeft(halfWidth);
        inputViewport->setBounds(inputArea.reduced(5, 5));
        
        // Output viewport on RIGHT HALF (50% to 100%)
        outputViewport->setBounds(meterArea.reduced(5, 5));
    }
    
    // Reserved area for future modules (empty for now)
    // bounds now contains the empty reserved area
}

MainWindow::MainWindow(juce::String name)
    : DocumentWindow(name,
                    AudioCoPilot::DesignSystem::Colours::getColour(AudioCoPilot::DesignSystem::Colours::Surface::Background),
                    DocumentWindow::allButtons)
{
    // Create Look and Feel first
    lookAndFeel = std::make_unique<AudioCoPilot::IndustrialLookAndFeel>();
    juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeel.get());
    
    // Create core components
    deviceStateModel = std::make_unique<DeviceStateModel>();
    deviceManager = std::make_unique<DeviceManager>(*deviceStateModel);
    audioEngine = std::make_unique<AudioEngine>(*deviceManager, *deviceStateModel);
    
    // Initialize audio engine
    audioEngine->initialize();
    
    // Create content component
    contentComponent = std::make_unique<MainContentComponent>();
    setContentNonOwned(contentComponent.get(), true);
    
    // Create UI components
    deviceSelector = std::make_unique<DeviceSelectorComponent>(*deviceManager);
    inputMeters = std::make_unique<ChannelMeterComponent>(*deviceStateModel, true);
    outputMeters = std::make_unique<ChannelMeterComponent>(*deviceStateModel, false);
    
    // Create viewports for scrolling
    inputViewport = std::make_unique<juce::Viewport>();
    outputViewport = std::make_unique<juce::Viewport>();
    inputViewport->setViewedComponent(inputMeters.get(), false);
    outputViewport->setViewedComponent(outputMeters.get(), false);
    
    // Link to content component
    contentComponent->deviceSelector = deviceSelector.get();
    contentComponent->inputMeters = inputMeters.get();
    contentComponent->outputMeters = outputMeters.get();
    contentComponent->inputViewport = inputViewport.get();
    contentComponent->outputViewport = outputViewport.get();
    
    // Create Transfer Function controller
    tfController = std::make_unique<TFController>(*deviceManager);

    // Create Anti-Masking controller (depends on Phase 1 device)
    antiMaskingController = std::make_unique<AudioCoPilot::AntiMaskingController>(*deviceManager);

    // Create RTA controller
    rtaController = std::make_unique<AudioCoPilot::RTAController>(*deviceManager);

    // Create AI Stage Hand controller
    aiStageHandController = std::make_unique<AudioCoPilot::AIStageHandController>(*deviceManager);
    
    // Create menu bar
    menuBarModel = std::make_unique<MenuBarModel>(*deviceManager);
    menuBarModel->setModuleActivationCallback([this](int moduleID) {
        if (moduleID == 30) // TransferFunction
        {
            showTransferFunction();
        }
        else if (moduleID == 31) // AntiMasking
        {
            showAntiMasking();
        }
        else if (moduleID == 32) // RTA
        {
            showRTA();
        }
        else if (moduleID == 33) // AI Stage Hand
        {
            showAIStageHand();
        }
    });
    menuBar = std::make_unique<juce::MenuBarComponent>(menuBarModel.get());
    contentComponent->menuBar = menuBar.get();
    
    // Setup UI
    setupUI();
    
    // Listen to localization changes
    LocalizedStrings::getInstance().addChangeListener(this);
    
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    
    // Get available screen area
    auto screenBounds = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
    int maxWidth = screenBounds.getWidth() - 40;  // Leave 20px margin on each side
    int maxHeight = screenBounds.getHeight() - 40;  // Leave 20px margin top/bottom
    
    // Calculate required window size for up to 64 channels
    // Each meter: 12px height + 2px spacing = 14px per channel
    // 64 channels * 14px = 896px + title (20px) + selectors (80px) + padding (40px) = ~1036px
    int requiredHeight = 80 + 20 + (64 * 14) + 40;  // selectors + title + meters + padding
    int requiredWidth = 1600;  // Wide enough for side-by-side meters
    
    // Use screen size or required size, whichever is smaller
    int windowWidth = juce::jmin(requiredWidth, maxWidth);
    int windowHeight = juce::jmin(requiredHeight, maxHeight);
    
    // Center window on screen
    int windowX = (screenBounds.getWidth() - windowWidth) / 2;
    int windowY = (screenBounds.getHeight() - windowHeight) / 2;
    
    setBounds(windowX, windowY, windowWidth, windowHeight);
    
    setVisible(true);
}

MainWindow::~MainWindow()
{
    LocalizedStrings::getInstance().removeChangeListener(this);
    
    // Shutdown audio
    hideAIStageHand();
    audioEngine->shutdown();
    
    // Remove components
    hideTransferFunction();
    inputViewport = nullptr;
    outputViewport = nullptr;
    deviceSelector = nullptr;
    inputMeters = nullptr;
    outputMeters = nullptr;
    tfController = nullptr;
    rtaController = nullptr;
    menuBar = nullptr;
    menuBarModel = nullptr;
    
    // Cleanup core
    audioEngine = nullptr;
    deviceManager = nullptr;
    deviceStateModel = nullptr;
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainWindow::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &LocalizedStrings::getInstance())
    {
        // Update window title if needed
        repaint();
    }
}

void MainWindow::setupUI()
{
    // Add components to content component
    contentComponent->addAndMakeVisible(deviceSelector.get());
    contentComponent->addAndMakeVisible(inputViewport.get());
    contentComponent->addAndMakeVisible(outputViewport.get());
    
    // Set menu bar (macOS)
    #if JUCE_MAC
    setMenuBar(menuBarModel.get());
    #else
    contentComponent->addAndMakeVisible(menuBar.get());
    #endif
}

void MainWindow::showTransferFunction()
{
    // Ensure other module is hidden
    hideAntiMasking();
    hideRTA();
    hideAIStageHand();

    if (tfView != nullptr)
    {
        // Already showing, hide it
        hideTransferFunction();
        return;
    }
    
    // Create and show Transfer Function view
    tfView = std::make_unique<TransferFunctionView>(*tfController);
    contentComponent->addAndMakeVisible(tfView.get());
    
    // Activate controller
    tfController->activate();
    
    // Hide meters, show TF view
    if (inputViewport != nullptr) inputViewport->setVisible(false);
    if (outputViewport != nullptr) outputViewport->setVisible(false);
    if (deviceSelector != nullptr) deviceSelector->setVisible(false);
    
    contentComponent->resized();
}

void MainWindow::hideTransferFunction()
{
    if (tfView == nullptr)
        return;
    
    // Deactivate controller
    if (tfController != nullptr)
        tfController->deactivate();
    
    // Remove view
    contentComponent->removeChildComponent(tfView.get());
    tfView = nullptr;
    
    // Show meters again
    if (inputViewport != nullptr) inputViewport->setVisible(true);
    if (outputViewport != nullptr) outputViewport->setVisible(true);
    if (deviceSelector != nullptr) deviceSelector->setVisible(true);
    
    contentComponent->resized();
}

void MainWindow::showAntiMasking()
{
    // Ensure other module is hidden
    hideTransferFunction();
    hideRTA();
    hideAIStageHand();

    if (antiMaskingView != nullptr)
    {
        hideAntiMasking();
        return;
    }

    antiMaskingView = std::make_unique<AudioCoPilot::AntiMaskingView>(*antiMaskingController);
    contentComponent->addAndMakeVisible(antiMaskingView.get());

    antiMaskingController->activate();

    if (inputViewport != nullptr) inputViewport->setVisible(false);
    if (outputViewport != nullptr) outputViewport->setVisible(false);
    if (deviceSelector != nullptr) deviceSelector->setVisible(false);

    contentComponent->resized();
}

void MainWindow::hideAntiMasking()
{
    if (antiMaskingView == nullptr)
        return;

    if (antiMaskingController != nullptr)
        antiMaskingController->deactivate();

    contentComponent->removeChildComponent(antiMaskingView.get());
    antiMaskingView = nullptr;

    if (inputViewport != nullptr) inputViewport->setVisible(true);
    if (outputViewport != nullptr) outputViewport->setVisible(true);
    if (deviceSelector != nullptr) deviceSelector->setVisible(true);

    contentComponent->resized();
}

void MainWindow::showRTA()
{
    hideTransferFunction();
    hideAntiMasking();
    hideAIStageHand();

    if (rtaView != nullptr)
    {
        hideRTA();
        return;
    }

    rtaView = std::make_unique<AudioCoPilot::RTAView>(*rtaController, *deviceManager);
    contentComponent->addAndMakeVisible(rtaView.get());

    rtaController->activate();

    if (inputViewport != nullptr) inputViewport->setVisible(false);
    if (outputViewport != nullptr) outputViewport->setVisible(false);
    if (deviceSelector != nullptr) deviceSelector->setVisible(false);

    contentComponent->resized();
}

void MainWindow::hideRTA()
{
    if (rtaView == nullptr)
        return;

    if (rtaController != nullptr)
        rtaController->deactivate();

    contentComponent->removeChildComponent(rtaView.get());
    rtaView = nullptr;

    if (inputViewport != nullptr) inputViewport->setVisible(true);
    if (outputViewport != nullptr) outputViewport->setVisible(true);
    if (deviceSelector != nullptr) deviceSelector->setVisible(true);

    contentComponent->resized();
}

void MainWindow::showAIStageHand()
{
    hideTransferFunction();
    hideAntiMasking();
    hideRTA();

    if (aiStageHandView != nullptr)
    {
        hideAIStageHand();
        return;
    }

    aiStageHandView = std::make_unique<AudioCoPilot::AIStageHandView>(*aiStageHandController);
    contentComponent->addAndMakeVisible(aiStageHandView.get());

    aiStageHandController->activate();

    if (inputViewport != nullptr) inputViewport->setVisible(false);
    if (outputViewport != nullptr) outputViewport->setVisible(false);
    if (deviceSelector != nullptr) deviceSelector->setVisible(false);

    contentComponent->resized();
}

void MainWindow::hideAIStageHand()
{
    if (aiStageHandView == nullptr)
        return;

    if (aiStageHandController != nullptr)
        aiStageHandController->deactivate();

    contentComponent->removeChildComponent(aiStageHandView.get());
    aiStageHandView = nullptr;

    if (inputViewport != nullptr) inputViewport->setVisible(true);
    if (outputViewport != nullptr) outputViewport->setVisible(true);
    if (deviceSelector != nullptr) deviceSelector->setVisible(true);

    contentComponent->resized();
}
