cmake_minimum_required(VERSION 3.22)

project(AudioCoPilot VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# JUCE Configuration
add_subdirectory(JUCE)

# Application Target
juce_add_gui_app(AudioCoPilot
    COMPANY_NAME "AudioCoPilot"
    PRODUCT_NAME "Audio Co-Pilot"
    VERSION "1.0.0"
    BUILD_VERSION "20260115.1"
    BUNDLE_ID "com.audiocopilot.app"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Audio Co-Pilot precisa de acesso ao microfone para processar áudio em tempo real."
)

# Embed icon + logo as official bundle resources (more reliable than post-build copying)
if(APPLE)
    set(APP_ICON_ICNS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Brand/Icon.icns")
    set(APP_LOGO_PNG  "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Brand/AudioCoPilotLogo.png")

    set_source_files_properties("${APP_ICON_ICNS}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties("${APP_LOGO_PNG}"  PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    target_sources(AudioCoPilot PRIVATE "${APP_ICON_ICNS}" "${APP_LOGO_PNG}")

    # Make sure Info.plist points to this file name
    set_target_properties(AudioCoPilot PROPERTIES MACOSX_BUNDLE_ICON_FILE "Icon.icns")
endif()

# Source Files
target_sources(AudioCoPilot PRIVATE
    Source/Main.cpp
    Source/UI/MacDockIcon.mm
    Source/UI/MainWindow.cpp
    Source/UI/MainWindow.h
    
    # Core
    Source/Core/DeviceManager.cpp
    Source/Core/DeviceManager.h
    Source/Core/AudioEngine.cpp
    Source/Core/AudioEngine.h
    Source/Core/DeviceStateModel.cpp
    Source/Core/DeviceStateModel.h
    
    # UI
    Source/UI/DeviceSelectorComponent.cpp
    Source/UI/DeviceSelectorComponent.h
    Source/UI/ChannelMeterComponent.cpp
    Source/UI/ChannelMeterComponent.h
    
    # Design System
    Source/UI/DesignSystem/DesignSystem.h
    Source/UI/DesignSystem/Colours.h
    Source/UI/DesignSystem/Typography.h
    Source/UI/DesignSystem/Spacing.h
    
    # Look and Feel
    Source/UI/LookAndFeel/IndustrialLookAndFeel.cpp
    Source/UI/LookAndFeel/IndustrialLookAndFeel.h
    
    # Transfer Function Module
    Source/Core/TransferFunction/FFTAnalyzer.cpp
    Source/Core/TransferFunction/FFTAnalyzer.h
    Source/Core/TransferFunction/TFProcessor.cpp
    Source/Core/TransferFunction/TFProcessor.h
    Source/Core/TransferFunction/TFController.cpp
    Source/Core/TransferFunction/TFController.h
    Source/Core/TransferFunction/TFAutoAnalyzer.cpp
    Source/Core/TransferFunction/TFAutoAnalyzer.h
    Source/Core/TransferFunction/TFKnowledgeBase.cpp
    Source/Core/TransferFunction/TFKnowledgeBase.h
    Source/UI/TransferFunction/PhasePlotComponent.cpp
    Source/UI/TransferFunction/PhasePlotComponent.h
    Source/UI/TransferFunction/MagnitudePlotComponent.cpp
    Source/UI/TransferFunction/MagnitudePlotComponent.h
    Source/UI/TransferFunction/TransferFunctionView.cpp
    Source/UI/TransferFunction/TransferFunctionView.h
    Source/UI/TransferFunction/TFAutoSuggestionsComponent.cpp
    Source/UI/TransferFunction/TFAutoSuggestionsComponent.h

    # Anti-Masking Module
    Source/Modules/AntiMasking/BarkAnalyzer.cpp
    Source/Modules/AntiMasking/BarkAnalyzer.h
    Source/Modules/AntiMasking/SpreadingFunction.cpp
    Source/Modules/AntiMasking/SpreadingFunction.h
    Source/Modules/AntiMasking/MaskingCalculator.cpp
    Source/Modules/AntiMasking/MaskingCalculator.h
    Source/Modules/AntiMasking/AntiMaskingProcessor.cpp
    Source/Modules/AntiMasking/AntiMaskingProcessor.h
    Source/Modules/AntiMasking/AntiMaskingController.cpp
    Source/Modules/AntiMasking/AntiMaskingController.h
    Source/Modules/AntiMasking/MaskingMatrixDisplay.h
    Source/Modules/AntiMasking/AISuggestionsEngine.h
    Source/Modules/AntiMasking/BarkSpectrumDisplay.h
    Source/Modules/AntiMasking/AntiMaskingView.cpp
    Source/Modules/AntiMasking/AntiMaskingView.h

    # RTA Module
    Source/Modules/RTA/RTAProcessor.cpp
    Source/Modules/RTA/RTAProcessor.h
    Source/Modules/RTA/RTAController.cpp
    Source/Modules/RTA/RTAController.h
    Source/Modules/RTA/RTAView.cpp
    Source/Modules/RTA/RTAView.h

    # AI Stage Hand Module
    Source/Modules/AIStageHand/AIStageHandFifo.cpp
    Source/Modules/AIStageHand/AIStageHandFifo.h
    Source/Modules/AIStageHand/AIStageHandAnalyzer.cpp
    Source/Modules/AIStageHand/AIStageHandAnalyzer.h
    Source/Modules/AIStageHand/AIStageHandController.cpp
    Source/Modules/AIStageHand/AIStageHandController.h
    Source/Modules/AIStageHand/AIStageHandView.cpp
    Source/Modules/AIStageHand/AIStageHandView.h
    
    # Menu
    Source/Menu/MenuBarModel.cpp
    Source/Menu/MenuBarModel.h
    
    # Localization
    Source/Localization/LocalizedStrings.cpp
    Source/Localization/LocalizedStrings.h
)

# JUCE Modules
target_link_libraries(AudioCoPilot PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

# macOS Specific
if(APPLE)
    target_link_libraries(AudioCoPilot PRIVATE
        "-framework CoreAudio"
        "-framework CoreAudioKit"
        "-framework AudioToolbox"
    )
endif()
