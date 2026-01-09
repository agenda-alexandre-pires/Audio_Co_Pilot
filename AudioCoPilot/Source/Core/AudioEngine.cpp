#include "AudioEngine.h"
#include "DebugLogger.h"
#include "../Modules/TransferFunction/TransferFunctionProcessor.h"
#include "../Modules/FeedbackPrediction/FeedbackPredictionProcessor.h"
#include "../Modules/AntiMasking/AntiMaskingProcessor.h"
#include <cmath>
#include <algorithm>

AudioEngine::AudioEngine(DeviceManager& deviceManager)
    : _deviceManager(deviceManager)
{
    DEBUG_LOG("AudioEngine constructor called");
    
    // Inicializa arrays atomic
    for (int i = 0; i < MAX_CHANNELS; ++i)
    {
        _peakLevels[i] = 0.0f;
        _rmsLevels[i] = 0.0f;
        _clipping[i] = false;
    }
    
    // Inicializa tracking de tempo
    _lastAudioCallbackTime.store(std::chrono::steady_clock::now());
    _deviceError.store(false);
    _restartAttempts.store(0);
    
    // Cria processador de transfer function (inicialmente desabilitado)
    _transferFunctionProcessor = std::make_unique<TransferFunctionProcessor>();
    _transferFunctionProcessor->setEnabled(false);
    DEBUG_LOG("TransferFunctionProcessor created (disabled)");
    
    // Cria processador de feedback prediction APENAS quando necessário
    // Inicialmente nullptr - será criado sob demanda
    _feedbackPredictionProcessor = nullptr;
    DEBUG_LOG("AudioEngine initialized successfully");
}

AudioEngine::~AudioEngine()
{
    DEBUG_LOG("AudioEngine destructor called");
    stop();
    DEBUG_LOG("AudioEngine destroyed");
}

void AudioEngine::start()
{
    DEBUG_LOG("AudioEngine::start() called");
    
    if (!_running)
    {
        DEBUG_LOG("Adding audio callback to device manager...");
        try {
            _deviceManager.getAudioDeviceManager().addAudioCallback(this);
            _running = true;
            DEBUG_LOG("AudioEngine started successfully");
        }
        catch (const std::exception& e) {
            DEBUG_ERROR("EXCEPTION in AudioEngine::start: " + juce::String(e.what()));
            throw;
        }
        catch (...) {
            DEBUG_ERROR("UNKNOWN EXCEPTION in AudioEngine::start");
            throw;
        }
    }
    else
    {
        DEBUG_LOG("AudioEngine already running, skipping start");
    }
}

void AudioEngine::stop()
{
    DEBUG_LOG("AudioEngine::stop() called");
    
    if (_running)
    {
        DEBUG_LOG("Removing audio callback from device manager...");
        try {
            _running = false;
            _deviceManager.getAudioDeviceManager().removeAudioCallback(this);
            DEBUG_LOG("AudioEngine stopped successfully");
        }
        catch (const std::exception& e) {
            DEBUG_ERROR("EXCEPTION in AudioEngine::stop: " + juce::String(e.what()));
            throw;
        }
        catch (...) {
            DEBUG_ERROR("UNKNOWN EXCEPTION in AudioEngine::stop");
            throw;
        }
    }
    else
    {
        DEBUG_LOG("AudioEngine already stopped, skipping stop");
    }
}

std::vector<ChannelLevels> AudioEngine::getCurrentLevels()
{
    int numCh = _numChannels.load();
    std::vector<ChannelLevels> levels(static_cast<size_t>(numCh));
    
    for (int i = 0; i < numCh; ++i)
    {
        levels[static_cast<size_t>(i)].peakLevel = _peakLevels[i].load();
        levels[static_cast<size_t>(i)].rmsLevel = _rmsLevels[i].load();
        levels[static_cast<size_t>(i)].clipping = _clipping[i].load();
    }
    
    return levels;
}

std::vector<ChannelLevels> AudioEngine::readAndResetLevels()
{
    int numCh = _numChannels.load();
    std::vector<ChannelLevels> levels(static_cast<size_t>(numCh));
    
    for (int i = 0; i < numCh; ++i)
    {
        // exchange() lê o valor atual E seta para 0.0 atomicamente
        float peak = _peakLevels[i].exchange(0.0f, std::memory_order_acquire);
        float rms = _rmsLevels[i].exchange(0.0f, std::memory_order_acquire);
        bool clip = _clipping[i].exchange(false, std::memory_order_acquire);
        
        // Retorna em LINEAR (não em dB) - a conversão será feita na UI
        levels[static_cast<size_t>(i)].peakLevel = peak;
        levels[static_cast<size_t>(i)].rmsLevel = rms;
        levels[static_cast<size_t>(i)].clipping = clip;
    }
    
    return levels;
}

int AudioEngine::getNumInputChannels() const
{
    return _numChannels.load();
}

void AudioEngine::enableTransferFunction(bool enable)
{
    if (_transferFunctionProcessor)
        _transferFunctionProcessor->setEnabled(enable);
}

void AudioEngine::enableFeedbackPrediction(bool enable)
{
    if (enable)
    {
        // CORREÇÃO: Não cria o processador aqui - deve ser criado pelo thread da UI
        // Apenas habilita se já existir
        if (_feedbackPredictionProcessor)
        {
            _feedbackPredictionProcessor->setEnabled(true);
        }
        else
        {
            DEBUG_WARNING("FeedbackPredictionProcessor not created yet - call createFeedbackPredictionProcessor() first");
        }
    }
    else if (_feedbackPredictionProcessor)
    {
        _feedbackPredictionProcessor->setEnabled(false);
    }
}

bool AudioEngine::isDeviceActive() const
{
    if (!_running.load())
        return false;
    
    auto lastCallback = _lastAudioCallbackTime.load();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCallback);
    
    // Se não houve callback nos últimos 2 segundos, dispositivo está inativo
    return elapsed.count() < 2000;
}

void AudioEngine::attemptRestart()
{
    if (_restartAttempts.load() >= MAX_RESTART_ATTEMPTS)
    {
        std::cerr << "MAX restart attempts reached. Giving up." << std::endl;
        return;
    }
    
    _restartAttempts++;
    
    std::cerr << "Attempting to restart audio device (attempt " 
              << _restartAttempts.load() << " of " << MAX_RESTART_ATTEMPTS << ")" << std::endl;
    
    // Para o dispositivo atual
    stop();
    
    // Pequena pausa para o sistema se recuperar
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Tenta reiniciar
    start();
    
    if (_running.load())
    {
        std::cerr << "Audio device restart successful!" << std::endl;
        _deviceError.store(false);
        _restartAttempts.store(0);
    }
    else
    {
        std::cerr << "Audio device restart failed." << std::endl;
    }
}

AudioCoPilot::FeedbackPredictionProcessor* AudioEngine::getFeedbackPredictionProcessor()
{
    return _feedbackPredictionProcessor.get();
}

void AudioEngine::createFeedbackPredictionProcessor()
{
    // CORREÇÃO: Criação thread-safe, deve ser chamada do thread da UI
    if (!_feedbackPredictionProcessor)
    {
        DEBUG_LOG("Creating FeedbackPredictionProcessor...");
        _feedbackPredictionProcessor = std::make_unique<AudioCoPilot::FeedbackPredictionProcessor>();
        
        // Prepara com as configurações atuais
        if (_sampleRate.load() > 0)
        {
            int maxBlockSize = 1024; // Valor conservador
            _feedbackPredictionProcessor->prepare(_sampleRate.load(), maxBlockSize);
            DEBUG_LOG("FeedbackPredictionProcessor prepared with sample rate: " + 
                     juce::String(_sampleRate.load()));
        }
        else
        {
            DEBUG_WARNING("Sample rate not set yet, FeedbackPredictionProcessor will be prepared later");
        }
    }
}

AudioCoPilot::AntiMaskingProcessor* AudioEngine::getAntiMaskingProcessor()
{
    return _antiMaskingProcessor.get();
}

void AudioEngine::createAntiMaskingProcessor()
{
    // CORREÇÃO: Criação thread-safe, deve ser chamada do thread da UI
    if (!_antiMaskingProcessor)
    {
        DEBUG_LOG("Creating AntiMaskingProcessor...");
        _antiMaskingProcessor = std::make_unique<AudioCoPilot::AntiMaskingProcessor>();
        
        // Prepara com as configurações atuais
        if (_sampleRate.load() > 0)
        {
            int maxBlockSize = 1024; // Valor conservador
            _antiMaskingProcessor->prepare(_sampleRate.load(), maxBlockSize);
            DEBUG_LOG("AntiMaskingProcessor prepared with sample rate: " + 
                     juce::String(_sampleRate.load()));
        }
        else
        {
            DEBUG_WARNING("Sample rate not set yet, AntiMaskingProcessor will be prepared later");
        }
    }
}

void AudioEngine::enableAntiMasking(bool enable)
{
    _antiMaskingEnabled.store(enable);
    if (_antiMaskingProcessor)
    {
        // O processador não tem método setEnabled, então apenas controlamos via flag
        DEBUG_LOG("AntiMasking " + juce::String(enable ? "enabled" : "disabled"));
    }
    else if (enable)
    {
        DEBUG_WARNING("AntiMaskingProcessor not created yet - call createAntiMaskingProcessor() first");
    }
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                    int numInputChannels,
                                                    float* const* outputChannelData,
                                                    int numOutputChannels,
                                                    int numSamples,
                                                    const juce::AudioIODeviceCallbackContext& context)
{
    // CORREÇÃO: Removido DEBUG_PROFILE para evitar travamentos
    // Callback de áudio deve ser ULTRA-RÁPIDO e sem operações bloqueantes
    
    juce::ignoreUnused(context);
    
    // EMERGENCY MODE: Se estamos em modo de emergência, skip processamento pesado
    if (_emergencyMode.load())
    {
        // Apenas processa metering básico
        if (inputChannelData != nullptr && numInputChannels > 0)
        {
            processMetering(inputChannelData, numInputChannels, numSamples);
        }
        return;
    }
    
    // Reset error counter se callback está OK
    _consecutiveCallbackErrors.store(0);
    
    // DEBUG: Log entrada do callback - APENAS os primeiros 3 para diagnóstico
    // Logs frequentes podem causar travamento!
    static int callbackCount = 0;
    callbackCount++;
    
    // Log apenas os primeiros 3 callbacks
    if (callbackCount <= 3)
    {
        std::cout << "[AudioEngine] Callback #" << callbackCount 
                  << " - channels: " << numInputChannels 
                  << ", samples: " << numSamples << std::endl;
        
        if (inputChannelData != nullptr && numInputChannels > 0 && inputChannelData[0] != nullptr)
        {
            // Verifica se há dados não-zero no primeiro canal
            bool hasData = false;
            for (int i = 0; i < std::min(numSamples, 10); ++i)
            {
                if (std::abs(inputChannelData[0][i]) > 0.0001f)
                {
                    hasData = true;
                    std::cout << "[AudioEngine]   Sample[0] = " << inputChannelData[0][i] << std::endl;
                    break;
                }
            }
            if (!hasData)
            {
                std::cout << "[AudioEngine]   WARNING: All samples are zero!" << std::endl;
            }
        }
        
        if (callbackCount == 3)
        {
            std::cout << "[AudioEngine] First 3 callbacks logged. Stopping logs to avoid performance issues." << std::endl;
        }
    }
    
    // Atualiza timestamp da última callback (dispositivo está ativo)
    _lastAudioCallbackTime.store(std::chrono::steady_clock::now());
    
    // Zera saída (não queremos feedback!)
    for (int ch = 0; ch < numOutputChannels; ++ch)
    {
        if (outputChannelData[ch] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
    }
    
    // Processa metering
    if (inputChannelData != nullptr && numInputChannels > 0)
    {
        processMetering(inputChannelData, numInputChannels, numSamples);
        
        // Processa transfer function (precisa de pelo menos 2 canais)
        if (_transferFunctionProcessor && numInputChannels >= 2)
        {
            _transferFunctionProcessor->processBlock(inputChannelData, numInputChannels, numSamples);
        }
        
        // Processa feedback prediction (AGORA COM REFERÊNCIA SE DISPONÍVEL)
        if (_feedbackPredictionProcessor && _feedbackPredictionProcessor->isEnabled() && numInputChannels > 0 && inputChannelData[0] != nullptr)
        {
            try
            {
                if (numInputChannels >= 2 && inputChannelData[1] != nullptr) {
                    // Modo 2 Canais: Ch 0 = Mic, Ch 1 = Console
                    _feedbackPredictionProcessor->process(inputChannelData[0], inputChannelData[1], numSamples);
                } else {
                    // Modo 1 Canal: Ch 0 = Mic (Fallback para análise simples)
                    _feedbackPredictionProcessor->process(inputChannelData[0], nullptr, numSamples);
                }
            }
            catch (const std::exception& e)
            {
                // Erro silencioso - não pode bloquear o callback
            }
            catch (...)
            {
                // Erro silencioso
            }
        }
        
        // Processa anti-masking (precisa de múltiplos canais)
        if (_antiMaskingProcessor && _antiMaskingEnabled.load() && numInputChannels > 0)
        {
            try
            {
                // Cria AudioBuffer temporário a partir dos dados de entrada
                juce::AudioBuffer<float> tempBuffer(numInputChannels, numSamples);
                for (int ch = 0; ch < numInputChannels; ++ch)
                {
                    if (inputChannelData[ch] != nullptr)
                    {
                        tempBuffer.copyFrom(ch, 0, inputChannelData[ch], numSamples);
                    }
                }
                
                _antiMaskingProcessor->processBlock(tempBuffer);
            }
            catch (const std::exception& e)
            {
                // Erro silencioso - não pode bloquear o callback
            }
            catch (...)
            {
                // Erro silencioso
            }
        }
    }
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    std::cout << "[AudioEngine] audioDeviceAboutToStart() called" << std::endl;
    
    if (device != nullptr)
    {
        std::cout << "[AudioEngine] Device: " << device->getName().toStdString() << std::endl;
        
        double sampleRate = device->getCurrentSampleRate();
        _sampleRate = sampleRate;
        std::cout << "[AudioEngine] Sample rate: " << sampleRate << " Hz" << std::endl;
        
        int activeChannels = device->getActiveInputChannels().countNumberOfSetBits();
        _numChannels = std::min(activeChannels, MAX_CHANNELS);
        std::cout << "[AudioEngine] Active input channels: " << activeChannels << std::endl;
        std::cout << "[AudioEngine] Buffer size: " << device->getCurrentBufferSizeSamples() << " samples" << std::endl;
        
        // Reset níveis
        for (int i = 0; i < MAX_CHANNELS; ++i)
        {
            _peakLevels[i] = 0.0f;
            _rmsLevels[i] = 0.0f;
            _clipping[i] = false;
        }
        
        std::cout << "[AudioEngine] audioDeviceAboutToStart() completed successfully" << std::endl;
        
        // Prepara transfer function processor
        if (_transferFunctionProcessor)
        {
            int maxBlockSize = device->getCurrentBufferSizeSamples();
            _transferFunctionProcessor->prepare(sampleRate, maxBlockSize);
        }
        
        // Prepara feedback prediction processor (se existir)
        if (_feedbackPredictionProcessor)
        {
            int maxBlockSize = device->getCurrentBufferSizeSamples();
            _feedbackPredictionProcessor->prepare(sampleRate, maxBlockSize);
        }
        
        // Prepara anti-masking processor (se existir)
        if (_antiMaskingProcessor)
        {
            int maxBlockSize = device->getCurrentBufferSizeSamples();
            _antiMaskingProcessor->prepare(sampleRate, maxBlockSize);
            std::cout << "[AudioEngine] AntiMaskingProcessor prepared" << std::endl;
        }
    }
}

void AudioEngine::audioDeviceStopped()
{
    DBG("Audio Device Stopped - resetting channel count");
    std::cerr << "INFO: Audio device stopped by system" << std::endl;
    _numChannels = 0;
}

void AudioEngine::audioDeviceError(const juce::String& errorMessage)
{
    DBG("Audio Device Error: " << errorMessage);
    // Log mais detalhado para diagnóstico
    std::cerr << "CRITICAL: Audio Device Error - " << errorMessage.toStdString() << std::endl;
    std::cerr << "Device may have stopped. Current state: running=" << _running.load() 
              << ", channels=" << _numChannels.load() << std::endl;
}

void AudioEngine::processMetering(const float* const* inputData, int numChannels, int numSamples)
{
    for (int ch = 0; ch < numChannels && ch < MAX_CHANNELS; ++ch)
    {
        if (inputData[ch] == nullptr)
            continue;
            
        const float* data = inputData[ch];
        
        // Encontra o pico absoluto do buffer - SEM smoothing, SEM decay
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            float absSample = std::abs(data[i]);
            if (absSample > peak)
                peak = absSample;
        }
        
        // Detecta clipping
        bool clip = (peak >= 1.0f);
        if (clip)
            _clipping[ch] = true;
        
        // Atualiza o atomic SÓ se o novo valor for MAIOR que o atual
        // Isso garante que entre frames de UI, guardamos o maior pico
        float oldPeak = _peakLevels[ch].load(std::memory_order_relaxed);
        while (peak > oldPeak && 
               !_peakLevels[ch].compare_exchange_weak(oldPeak, peak, 
                                                       std::memory_order_release,
                                                       std::memory_order_relaxed))
        {
            // Loop CAS - tenta de novo se outro thread modificou
        }
        
        // RMS simples para este buffer (sem smoothing)
        float sumSquares = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            sumSquares += data[i] * data[i];
        }
        float rms = (numSamples > 0) ? std::sqrt(sumSquares / static_cast<float>(numSamples)) : 0.0f;
        
        // Mesmo padrão CAS para RMS
        float oldRms = _rmsLevels[ch].load(std::memory_order_relaxed);
        while (rms > oldRms && 
               !_rmsLevels[ch].compare_exchange_weak(oldRms, rms,
                                                      std::memory_order_release,
                                                      std::memory_order_relaxed))
        {
            // Loop CAS
        }
    }
}
