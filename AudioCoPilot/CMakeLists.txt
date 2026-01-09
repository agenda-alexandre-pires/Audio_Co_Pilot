cmake_minimum_required(VERSION 3.24)

# Versão do projeto
file(READ "VERSION" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)

project(AudioCoPilot VERSION ${PROJECT_VERSION})

# C++20 obrigatório
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Otimizações para Apple Silicon
set(CMAKE_OSX_ARCHITECTURES "arm64")
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0")

# JUCE
set(JUCE_PATH "/Applications/JUCE")
add_subdirectory(${JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE)

# Opções do JUCE
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)

# Definições globais
add_compile_definitions(
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_MODAL_LOOPS_PERMITTED=1
)

# ============================================
# APLICAÇÃO PRINCIPAL COM PERMISSÃO DE MICROFONE
# ============================================

# Aplicação standalone
juce_add_gui_app(${PROJECT_NAME}
    VERSION "${PROJECT_VERSION}"
    BUNDLE_ID "com.emersonporfaaudio.audiocopilot"
    COMPANY_NAME "Emerson Porfa Audio"
    PRODUCT_NAME "Audio Co-Pilot"
    ICON_BIG "${CMAKE_SOURCE_DIR}/Resources/Icon.png"
    DOCUMENT_EXTENSIONS ""
    NEEDS_CURL FALSE
    NEEDS_WEB_BROWSER FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE  # PERMISSÃO HABILITADA!
    # Permissão de microfone HABILITADA - necessário para acesso ao áudio no macOS
    # O macOS solicitará permissão na primeira execução
)

# Arquivos fonte
file(GLOB_RECURSE SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_SOURCE_DIR}/Source/*.h"
)

# Adiciona arquivos do Anti-Masking Module explicitamente
set(ANTI_MASKING_SOURCES
    "${CMAKE_SOURCE_DIR}/Source/Modules/AntiMasking/BarkAnalyzer.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/AntiMasking/SpreadingFunction.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/AntiMasking/MaskingCalculator.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/AntiMasking/AntiMaskingProcessor.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/AntiMasking/MaskingMatrixDisplay.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/AntiMasking/BarkSpectrumDisplay.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/AntiMasking/ChannelSelector.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/AntiMasking/AntiMaskingComponent.cpp"
)

target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES} ${ANTI_MASKING_SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/Source"
    "${CMAKE_SOURCE_DIR}/Source/Core"
    "${CMAKE_SOURCE_DIR}/Source/UI"
    "${CMAKE_SOURCE_DIR}/Source/UI/LookAndFeel"
    "${CMAKE_SOURCE_DIR}/Source/UI/Components"
    "${CMAKE_SOURCE_DIR}/Source/DSP"
    "${CMAKE_SOURCE_DIR}/Source/Modules"
    "${CMAKE_SOURCE_DIR}/Source/Modules/AntiMasking"
    "${CMAKE_BINARY_DIR}/JuceLibraryCode"
)

# Módulos JUCE necessários
target_link_libraries(${PROJECT_NAME} PRIVATE
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
    juce::juce_opengl
)

# Frameworks do macOS
target_link_libraries(${PROJECT_NAME} PRIVATE
    "-framework Accelerate"
    "-framework Metal"
    "-framework MetalKit"
    "-framework CoreAudio"
    "-framework CoreMIDI"
    "-framework AudioToolbox"
)

# Flags de compilação otimizadas
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-ffast-math>
    $<$<CONFIG:Release>:-march=native>
)

# ============================================
# TESTES COM CATCH2
# ============================================

Include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.0
)

FetchContent_MakeAvailable(Catch2)

# Target de testes
add_executable(${PROJECT_NAME}_Tests
    "${CMAKE_SOURCE_DIR}/Tests/Main.cpp"
    "${CMAKE_SOURCE_DIR}/Tests/AudioEngineTests.cpp"
    "${CMAKE_SOURCE_DIR}/Tests/RingBufferTests.cpp"
    "${CMAKE_SOURCE_DIR}/Tests/TransferFunctionTests.cpp"
    "${CMAKE_SOURCE_DIR}/Tests/FeedbackPredictionTests.cpp"
    "${CMAKE_SOURCE_DIR}/Tests/BarkAnalyzerTests.cpp"
    "${CMAKE_SOURCE_DIR}/Tests/MaskingCalculatorTests.cpp"
    "${CMAKE_SOURCE_DIR}/Tests/AntiMaskingIntegrationTests.cpp"
    # Inclui arquivos do Source necessários para testes
    "${CMAKE_SOURCE_DIR}/Source/Core/AudioEngine.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Core/DeviceManager.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Core/RingBuffer.h"
    "${CMAKE_SOURCE_DIR}/Source/Modules/TransferFunction/TransferFunctionProcessor.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/FeedbackPrediction/HowlingDetector.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/FeedbackPrediction/FeedbackPredictionProcessor.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/FeedbackPrediction/FeedbackLogger.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/FeedbackPrediction/PerformanceMonitor.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/FeedbackPrediction/DataExporter.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/FeedbackPrediction/DynamicConfig.cpp"
    # Inclui arquivos do Anti-Masking Module para testes
    "${CMAKE_SOURCE_DIR}/Source/Modules/AntiMasking/BarkAnalyzer.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/AntiMasking/SpreadingFunction.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/AntiMasking/MaskingCalculator.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Modules/AntiMasking/AntiMaskingProcessor.cpp"
)

target_include_directories(${PROJECT_NAME}_Tests PRIVATE
    "${CMAKE_SOURCE_DIR}/Source"
    "${CMAKE_SOURCE_DIR}/Source/Core"
    "${CMAKE_SOURCE_DIR}/Source/UI"
    "${CMAKE_SOURCE_DIR}/Source/UI/Components"
    "${CMAKE_SOURCE_DIR}/Source/DSP"
    "${CMAKE_SOURCE_DIR}/Source/Modules"
    "${CMAKE_SOURCE_DIR}/Source/Modules/TransferFunction"
    "${CMAKE_SOURCE_DIR}/Source/Modules/AntiMasking"
    # Inclui diretórios do JUCE
    "${JUCE_PATH}/modules"
    "${CMAKE_BINARY_DIR}/JuceLibraryCode"
)

target_link_libraries(${PROJECT_NAME}_Tests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_core
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    "-framework Accelerate"
    "-framework CoreAudio"
    "-framework AudioToolbox"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework Carbon"
    "-framework CoreFoundation"
)

target_compile_definitions(${PROJECT_NAME}_Tests PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_compile_options(${PROJECT_NAME}_Tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
)

# CTest
include(CTest)
enable_testing()

# Descobre testes mas não executa durante o build
if(Catch2_FOUND)
    include(Catch)
    catch_discover_tests(${PROJECT_NAME}_Tests)
endif()

# REMOVIDA A VERSÃO SIMPLES PARA EVITAR CONFLITOS
# (mantemos apenas a versão principal com permissões corrigidas)