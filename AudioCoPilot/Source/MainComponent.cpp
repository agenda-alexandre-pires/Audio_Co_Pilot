#include "MainComponent.h"
#include "Core/DebugLogger.h"
#include "UI/Colours.h"
#include <iostream>

MainComponent::MainComponent()
{
    std::cout << "==========================================" << std::endl;
    std::cout << "AUDIO CO-PILOT - VERSÃO ESTÁVEL" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    try {
        // 1. Tamanho inicial
        setSize(1024, 768);
        
        // 2. Cria StatusBar (sempre primeiro)
        _statusBar = std::make_unique<StatusBar>();
        addAndMakeVisible(*_statusBar);
        _statusBar->setStatus("Starting...", StatusBar::StatusType::Info);
        
        // 3. Cria HeaderBar
        _headerBar = std::make_unique<HeaderBar>();
        addAndMakeVisible(*_headerBar);
        
        // 4. Cria MeterBridge (visível por padrão)
        _meterBridge = std::make_unique<MeterBridge>();
        addAndMakeVisible(*_meterBridge);
        
        // 5. Configura callbacks do HeaderBar - SIMPLES
        _headerBar->onMetersClicked = [this]() { 
            std::cout << "[MAIN] Switching to Meters" << std::endl;
            switchViewMode(ViewMode::Meters); 
        };
        _headerBar->onTransferFunctionClicked = [this]() { 
            std::cout << "[MAIN] Switching to TransferFunction" << std::endl;
            switchViewMode(ViewMode::TransferFunction); 
        };
        _headerBar->onFeedbackPredictionClicked = [this]() { 
            std::cout << "[MAIN] Switching to FeedbackPrediction" << std::endl;
            switchViewMode(ViewMode::FeedbackPrediction); 
        };
        _headerBar->onAntiMaskingClicked = [this]() { 
            std::cout << "[MAIN] Switching to AntiMasking" << std::endl;
            switchViewMode(ViewMode::AntiMasking); 
        };
        
        // 6. Cria DeviceManager (mas NÃO inicializa áudio ainda)
        _deviceManager = std::make_unique<DeviceManager>();
        _headerBar->setDeviceManager(_deviceManager.get());
        std::cout << "[MAIN] DeviceManager created (audio NOT initialized)" << std::endl;
        
        // Configura callback para iniciar áudio quando dispositivo for selecionado
        if (auto* deviceSelector = _headerBar->getDeviceSelector())
        {
            deviceSelector->onAudioShouldEnable = [this]() {
                std::cout << "[MAIN] Device selected, initializing audio..." << std::endl;
                initializeAudioOnce();
            };
            std::cout << "[MAIN] Audio initialization callback configured" << std::endl;
        }
        
        // 7. Timer para atualização
        startTimerHz(30); // 30 FPS
        
        _statusBar->setStatus("Ready - Select audio device", StatusBar::StatusType::Info);
        std::cout << "[MAIN] Constructor completed successfully" << std::endl;
        std::cout << "==========================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[MAIN] ERROR in constructor: " << e.what() << std::endl;
        _statusBar->setStatus("Error: " + juce::String(e.what()), StatusBar::StatusType::Error);
        throw;
    } catch (...) {
        std::cerr << "[MAIN] UNKNOWN ERROR in constructor" << std::endl;
        _statusBar->setStatus("Unknown error", StatusBar::StatusType::Error);
        throw;
    }
}

MainComponent::~MainComponent()
{
    std::cout << "[MAIN] Destructor called" << std::endl;
    stopTimer();
    
    // Para áudio se estiver rodando
    if (_audioEngine && _audioRunning)
    {
        std::cout << "[MAIN] Stopping audio engine..." << std::endl;
        _audioEngine->stop();
    }
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1A1A1A));
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Layout CONSISTENTE e SIMPLES
    if (_headerBar)
    {
        _headerBar->setBounds(bounds.removeFromTop(60));
    }
    
    if (_statusBar)
    {
        _statusBar->setBounds(bounds.removeFromBottom(40));
    }
    
    // Área central
    _centralBounds = bounds;
    
    // Define bounds para o componente ativo
    if (_currentViewMode == ViewMode::Meters && _meterBridge)
    {
        _meterBridge->setBounds(_centralBounds);
    }
    else if (_currentViewMode == ViewMode::TransferFunction && _transferFunctionComponent)
    {
        _transferFunctionComponent->setBounds(_centralBounds);
    }
    else if (_currentViewMode == ViewMode::FeedbackPrediction && _feedbackPredictionComponent)
    {
        _feedbackPredictionComponent->setBounds(_centralBounds);
    }
    else if (_currentViewMode == ViewMode::AntiMasking && _antiMaskingComponent)
    {
        _antiMaskingComponent->setBounds(_centralBounds);
    }
}

void MainComponent::timerCallback()
{
    // SEMPRE atualiza (para logs e possível atualização de meters)
    updateMeters();
    
    // Verifica saúde do dispositivo periodicamente
    static int healthCheckCount = 0;
    if (++healthCheckCount % 180 == 0) // A cada ~6 segundos
    {
        checkDeviceHealth();
    }
}

void MainComponent::setupAudio()
{
    // Método vazio - áudio será inicializado apenas quando necessário
}

void MainComponent::updateMeters()
{
    static int updateCount = 0;
    updateCount++;
    
    if (_audioEngine && _audioRunning)
    {
        // SEMPRE tenta ler níveis se áudio está rodando
        auto levels = _audioEngine->readAndResetLevels();
        
        // Log detalhado para debug
        if (updateCount % 90 == 0) // A cada ~3 segundos (30Hz * 3)
        {
            std::cout << "[MAIN] updateMeters:" << std::endl;
            std::cout << "  - Audio running: " << (_audioRunning ? "YES" : "NO") << std::endl;
            std::cout << "  - Current view: " << viewModeToString(_currentViewMode) << std::endl;
            std::cout << "  - Channels detected: " << levels.size() << std::endl;
            
            if (!levels.empty())
            {
                std::cout << "  - Channel 0: peak=" << levels[0].peakLevel 
                          << ", rms=" << levels[0].rmsLevel 
                          << ", clipping=" << (levels[0].clipping ? "YES" : "NO") << std::endl;
            }
            else
            {
                std::cout << "  - WARNING: No channels detected!" << std::endl;
                std::cout << "  - Possible issues:" << std::endl;
                std::cout << "    1. Device has no active input channels" << std::endl;
                std::cout << "    2. macOS microphone permission not granted" << std::endl;
                std::cout << "    3. Device not producing audio" << std::endl;
            }
        }
        
        // Atualiza MeterBridge APENAS se estamos no módulo Meters
        if (_currentViewMode == ViewMode::Meters)
        {
            _meterBridge->setLevels(levels);
        }
    }
    else if (updateCount % 120 == 0) // Log a cada 4 segundos se áudio não está rodando
    {
        std::cout << "[MAIN] updateMeters: Audio NOT running" << std::endl;
        std::cout << "  - _audioEngine: " << (_audioEngine ? "CREATED" : "NULL") << std::endl;
        std::cout << "  - _audioRunning: " << (_audioRunning ? "YES" : "NO") << std::endl;
    }
}

void MainComponent::switchViewMode(ViewMode mode)
{
    std::cout << "[MAIN] switchViewMode: " << viewModeToString(mode) << std::endl;
    
    // Se já está no modo solicitado, não faz nada
    if (_currentViewMode == mode)
    {
        std::cout << "[MAIN] Already in requested view mode" << std::endl;
        return;
    }
    
    // Se mudando PARA Meters DE outro módulo
    if (mode == ViewMode::Meters)
    {
        std::cout << "[MAIN] Switching to Meters view" << std::endl;
        
        // Esconde outros componentes
        if (_transferFunctionComponent) _transferFunctionComponent->setVisible(false);
        if (_feedbackPredictionComponent) _feedbackPredictionComponent->setVisible(false);
        if (_antiMaskingComponent) _antiMaskingComponent->setVisible(false);
        
        // Mostra MeterBridge
        _meterBridge->setVisible(true);
        _headerBar->setActiveModule("Meters");
        
        // Desabilita processadores se necessário
        if (_audioEngine)
        {
            _audioEngine->enableTransferFunction(false);
            _audioEngine->enableFeedbackPrediction(false);
            _audioEngine->enableAntiMasking(false);
        }
    }
    // Se mudando DE Meters PARA outro módulo
    else if (_currentViewMode == ViewMode::Meters && mode != ViewMode::Meters)
    {
        std::cout << "[MAIN] Leaving Meters view, initializing audio if needed..." << std::endl;
        
        // INICIALIZA ÁUDIO APENAS UMA VEZ quando sai de Meters
        if (!_audioRunning)
        {
            initializeAudioOnce();
        }
        
        // Se áudio não inicializou, não muda de view
        if (!_audioRunning)
        {
            std::cout << "[MAIN] Audio not initialized, staying in Meters" << std::endl;
            _statusBar->setStatus("Audio not ready", StatusBar::StatusType::Warning);
            return;
        }
        
        // Esconde MeterBridge
        _meterBridge->setVisible(false);
        
        // Configura novo módulo
        if (mode == ViewMode::TransferFunction)
        {
            std::cout << "[MAIN] Setting up TransferFunction view" << std::endl;
            
            // Cria componente se não existir
            if (!_transferFunctionComponent && _audioEngine)
            {
                _transferFunctionComponent = std::make_unique<TransferFunctionComponent>();
                addChildComponent(*_transferFunctionComponent);
                _transferFunctionComponent->setProcessor(_audioEngine->getTransferFunctionProcessor());
            }
            
            if (_transferFunctionComponent)
            {
                _transferFunctionComponent->setVisible(true);
                _audioEngine->enableTransferFunction(true);
            }
            
            _headerBar->setActiveModule("TransferFunction");
        }
        else if (mode == ViewMode::FeedbackPrediction)
        {
            std::cout << "[MAIN] Setting up FeedbackPrediction view" << std::endl;
            
            // Cria componente se não existir
            if (!_feedbackPredictionComponent && _audioEngine)
            {
                std::cout << "[MAIN] Creating FeedbackPredictionProcessor..." << std::endl;
                // Cria processador primeiro
                _audioEngine->createFeedbackPredictionProcessor();
                
                auto* processor = _audioEngine->getFeedbackPredictionProcessor();
                if (processor)
                {
                    std::cout << "[MAIN] FeedbackPredictionProcessor created successfully" << std::endl;
                    _feedbackPredictionComponent = std::make_unique<AudioCoPilot::FeedbackPredictionComponent>(
                        *processor
                    );
                    addChildComponent(*_feedbackPredictionComponent);
                    std::cout << "[MAIN] FeedbackPredictionComponent created" << std::endl;
                }
                else
                {
                    std::cerr << "[MAIN] ERROR: Failed to create FeedbackPredictionProcessor!" << std::endl;
                }
            }
            
            if (_feedbackPredictionComponent)
            {
                std::cout << "[MAIN] Making FeedbackPredictionComponent visible and enabling processor" << std::endl;
                _feedbackPredictionComponent->setVisible(true);
                if (_audioEngine)
                {
                    _audioEngine->enableFeedbackPrediction(true);
                    std::cout << "[MAIN] FeedbackPrediction processor enabled" << std::endl;
                }
            }
            else
            {
                std::cerr << "[MAIN] ERROR: FeedbackPredictionComponent is null!" << std::endl;
            }
            
            _headerBar->setActiveModule("FeedbackPrediction");
        }
        else if (mode == ViewMode::AntiMasking)
        {
            std::cout << "[MAIN] Setting up AntiMasking view" << std::endl;
            
            // Cria componente se não existir
            if (!_antiMaskingComponent && _audioEngine)
            {
                std::cout << "[MAIN] Creating AntiMaskingProcessor..." << std::endl;
                // Cria processador primeiro
                _audioEngine->createAntiMaskingProcessor();
                
                auto* processor = _audioEngine->getAntiMaskingProcessor();
                if (processor)
                {
                    std::cout << "[MAIN] AntiMaskingProcessor created successfully" << std::endl;
                    _antiMaskingComponent = std::make_unique<AudioCoPilot::AntiMaskingComponent>();
                    _antiMaskingComponent->setProcessor(processor);
                    
                    // Atualiza número de canais no ChannelSelector
                    int numChannels = _audioEngine->getNumInputChannels();
                    if (numChannels > 0)
                    {
                        std::cout << "[MAIN] Setting " << numChannels << " channels in ChannelSelector" << std::endl;
                        // O ChannelSelector está dentro do AntiMaskingComponent, precisamos acessá-lo
                        // Por enquanto, vamos deixar o padrão (4 canais) e atualizar depois se necessário
                    }
                    
                    addChildComponent(*_antiMaskingComponent);
                    std::cout << "[MAIN] AntiMaskingComponent created and processor connected" << std::endl;
                }
                else
                {
                    std::cerr << "[MAIN] ERROR: Failed to create AntiMaskingProcessor!" << std::endl;
                }
            }
            
            if (_antiMaskingComponent)
            {
                std::cout << "[MAIN] Making AntiMaskingComponent visible and enabling processor" << std::endl;
                _antiMaskingComponent->setVisible(true);
                if (_audioEngine)
                {
                    _audioEngine->enableAntiMasking(true);
                    std::cout << "[MAIN] AntiMasking processor enabled" << std::endl;
                }
            }
            else
            {
                std::cerr << "[MAIN] ERROR: AntiMaskingComponent is null!" << std::endl;
            }
            
            _headerBar->setActiveModule("AntiMasking");
        }
    }
    // Se mudando entre módulos não-Meters
    else
    {
        std::cout << "[MAIN] Switching between non-Meters modules" << std::endl;
        
        // Esconde todos
        if (_transferFunctionComponent) _transferFunctionComponent->setVisible(false);
        if (_feedbackPredictionComponent) _feedbackPredictionComponent->setVisible(false);
        if (_antiMaskingComponent) _antiMaskingComponent->setVisible(false);
        
        // Desabilita processadores antigos
        if (_audioEngine)
        {
            if (_currentViewMode == ViewMode::TransferFunction)
                _audioEngine->enableTransferFunction(false);
            else if (_currentViewMode == ViewMode::FeedbackPrediction)
                _audioEngine->enableFeedbackPrediction(false);
            else if (_currentViewMode == ViewMode::AntiMasking)
                _audioEngine->enableAntiMasking(false);
        }
        
        // Configura novo módulo (mesmo código acima)
        if (mode == ViewMode::TransferFunction)
        {
            if (_transferFunctionComponent)
            {
                _transferFunctionComponent->setVisible(true);
                _audioEngine->enableTransferFunction(true);
            }
            _headerBar->setActiveModule("TransferFunction");
        }
        else if (mode == ViewMode::FeedbackPrediction)
        {
            // Cria componente se não existir
            if (!_feedbackPredictionComponent && _audioEngine)
            {
                std::cout << "[MAIN] Creating FeedbackPredictionProcessor (switching between modules)..." << std::endl;
                _audioEngine->createFeedbackPredictionProcessor();
                
                auto* processor = _audioEngine->getFeedbackPredictionProcessor();
                if (processor)
                {
                    _feedbackPredictionComponent = std::make_unique<AudioCoPilot::FeedbackPredictionComponent>(
                        *processor
                    );
                    addChildComponent(*_feedbackPredictionComponent);
                }
            }
            
            if (_feedbackPredictionComponent)
            {
                _feedbackPredictionComponent->setVisible(true);
                if (_audioEngine)
                {
                    _audioEngine->enableFeedbackPrediction(true);
                }
            }
            _headerBar->setActiveModule("FeedbackPrediction");
        }
        else if (mode == ViewMode::AntiMasking)
        {
            // Cria componente se não existir
            if (!_antiMaskingComponent && _audioEngine)
            {
                std::cout << "[MAIN] Creating AntiMaskingProcessor (switching between modules)..." << std::endl;
                _audioEngine->createAntiMaskingProcessor();
                
                auto* processor = _audioEngine->getAntiMaskingProcessor();
                if (processor)
                {
                    _antiMaskingComponent = std::make_unique<AudioCoPilot::AntiMaskingComponent>();
                    _antiMaskingComponent->setProcessor(processor);
                    addChildComponent(*_antiMaskingComponent);
                }
            }
            
            if (_antiMaskingComponent)
            {
                _antiMaskingComponent->setVisible(true);
                if (_audioEngine)
                {
                    _audioEngine->enableAntiMasking(true);
                }
            }
            _headerBar->setActiveModule("AntiMasking");
        }
    }
    
    _currentViewMode = mode;
    resized();
    repaint();
    
    std::cout << "[MAIN] switchViewMode completed successfully" << std::endl;
}

void MainComponent::checkDeviceHealth()
{
    // Método simplificado - apenas verifica se áudio está rodando
    if (_audioEngine && _audioRunning)
    {
        bool deviceActive = _audioEngine->isDeviceActive();
        if (!deviceActive)
        {
            _statusBar->setStatus("Audio Device Inactive", StatusBar::StatusType::Warning);
        }
    }
}

void MainComponent::initializeAudioOnce()
{
    std::cout << "[MAIN] initializeAudioOnce() called" << std::endl;
    
    if (_audioRunning)
    {
        std::cout << "[MAIN] Audio already running, skipping" << std::endl;
        return;
    }
    
    if (!_deviceManager)
    {
        std::cout << "[MAIN] ERROR: DeviceManager not created" << std::endl;
        _statusBar->setStatus("DeviceManager error", StatusBar::StatusType::Error);
        return;
    }
    
    _statusBar->setStatus("Initializing audio...", StatusBar::StatusType::Info);
    
    try {
        // 1. Verifica se DeviceManager já está inicializado
        // (pode ter sido inicializado por setDevice())
        bool deviceManagerNeedsInit = true;
        if (auto* currentDevice = _deviceManager->getAudioDeviceManager().getCurrentAudioDevice())
        {
            std::cout << "[MAIN] DeviceManager already initialized with device: " 
                      << currentDevice->getName().toStdString() << std::endl;
            deviceManagerNeedsInit = false;
        }
        
        // 2. Inicializa DeviceManager apenas se necessário
        if (deviceManagerNeedsInit)
        {
            std::cout << "[MAIN] Initializing DeviceManager..." << std::endl;
            if (!_deviceManager->initialise())
            {
                std::cout << "[MAIN] DeviceManager::initialise() failed" << std::endl;
                _statusBar->setStatus("Audio initialization failed", StatusBar::StatusType::Error);
                return;
            }
            std::cout << "[MAIN] DeviceManager initialized successfully" << std::endl;
        }
        
        // 2. Cria AudioEngine
        if (!_audioEngine)
        {
            std::cout << "[MAIN] Creating AudioEngine..." << std::endl;
            _audioEngine = std::make_unique<AudioEngine>(*_deviceManager);
        }
        
        // 3. Inicia AudioEngine
        std::cout << "[MAIN] Starting AudioEngine..." << std::endl;
        _audioEngine->start();
        _audioRunning = true;
        
        // 4. Conecta processadores aos componentes
        if (_transferFunctionComponent && _audioEngine->getTransferFunctionProcessor())
        {
            _transferFunctionComponent->setProcessor(_audioEngine->getTransferFunctionProcessor());
            std::cout << "[MAIN] TransferFunctionProcessor connected" << std::endl;
        }
        
        _statusBar->setStatus("Audio Ready", StatusBar::StatusType::Good);
        std::cout << "[MAIN] Audio initialized and started SUCCESSFULLY" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "[MAIN] EXCEPTION initializing audio: " << e.what() << std::endl;
        _statusBar->setStatus("Audio Error: " + juce::String(e.what()), StatusBar::StatusType::Error);
        _audioRunning = false;
    } catch (...) {
        std::cout << "[MAIN] UNKNOWN EXCEPTION initializing audio" << std::endl;
        _statusBar->setStatus("Unknown audio error", StatusBar::StatusType::Error);
        _audioRunning = false;
    }
}

juce::String MainComponent::viewModeToString(ViewMode mode)
{
    switch (mode) {
        case ViewMode::Meters: return "Meters";
        case ViewMode::TransferFunction: return "TransferFunction";
        case ViewMode::FeedbackPrediction: return "FeedbackPrediction";
        case ViewMode::AntiMasking: return "AntiMasking";
        default: return "Unknown";
    }
}


