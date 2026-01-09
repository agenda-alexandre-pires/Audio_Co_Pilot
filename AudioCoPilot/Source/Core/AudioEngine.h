#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <array>
#include <vector>
#include <chrono>
#include "DeviceManager.h"
#include "RingBuffer.h"

// Forward declaration
class TransferFunctionProcessor;
namespace AudioCoPilot { 
    class FeedbackPredictionProcessor;
    class AntiMaskingProcessor;
}

struct ChannelLevels
{
    float peakLevel = 0.0f;
    float rmsLevel = 0.0f;
    bool clipping = false;
};

class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    explicit AudioEngine(DeviceManager& deviceManager);
    ~AudioEngine() override;

    void start();
    void stop();
    bool isRunning() const { return _running; }

    // Obtém níveis atuais (thread-safe)
    std::vector<ChannelLevels> getCurrentLevels();
    
    // Lê os níveis e RESETA para zero (chamado pela UI)
    std::vector<ChannelLevels> readAndResetLevels();
    
    // Obtém número de canais ativos
    int getNumInputChannels() const;
    
    // Acesso ao processador de transfer function
    TransferFunctionProcessor* getTransferFunctionProcessor() { return _transferFunctionProcessor.get(); }
    
    // Acesso ao processador de feedback prediction (cria sob demanda, thread-safe)
    AudioCoPilot::FeedbackPredictionProcessor* getFeedbackPredictionProcessor();
    
    // Cria o processador de feedback prediction de forma thread-safe
    void createFeedbackPredictionProcessor();
    
    // Acesso ao processador de anti-masking (cria sob demanda, thread-safe)
    AudioCoPilot::AntiMaskingProcessor* getAntiMaskingProcessor();
    
    // Cria o processador de anti-masking de forma thread-safe
    void createAntiMaskingProcessor();
    
    // Controle de processadores
    void enableTransferFunction(bool enable);
    
    void enableFeedbackPrediction(bool enable);
    
    void enableAntiMasking(bool enable);
    
    // Verificação de saúde do dispositivo
    bool isDeviceActive() const;
    void attemptRestart();

    // AudioIODeviceCallback
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext& context) override;
    
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceError(const juce::String& errorMessage) override;

private:
    DeviceManager& _deviceManager;
    std::atomic<bool> _running { false };
    
    // Níveis por canal (lock-free)
    static constexpr int MAX_CHANNELS = 64;
    std::array<std::atomic<float>, MAX_CHANNELS> _peakLevels;
    std::array<std::atomic<float>, MAX_CHANNELS> _rmsLevels;
    std::array<std::atomic<bool>, MAX_CHANNELS> _clipping;
    
    std::atomic<int> _numChannels { 0 };
    std::atomic<double> _sampleRate { 48000.0 };
    
    // Tracking de atividade do dispositivo
    std::atomic<std::chrono::steady_clock::time_point> _lastAudioCallbackTime;
    std::atomic<bool> _deviceError { false };
    std::atomic<int> _restartAttempts { 0 };
    static constexpr int MAX_RESTART_ATTEMPTS = 3;
    
    // Emergency mode tracking
    std::atomic<int> _consecutiveCallbackErrors { 0 };
    static constexpr int MAX_CONSECUTIVE_ERRORS = 5;
    std::atomic<bool> _emergencyMode { false };
    
    // Processador de transfer function
    std::unique_ptr<TransferFunctionProcessor> _transferFunctionProcessor;
    
    // Processador de feedback prediction
    std::unique_ptr<AudioCoPilot::FeedbackPredictionProcessor> _feedbackPredictionProcessor;
    
    // Processador de anti-masking
    std::unique_ptr<AudioCoPilot::AntiMaskingProcessor> _antiMaskingProcessor;
    
    // Flag para controlar se anti-masking está habilitado
    std::atomic<bool> _antiMaskingEnabled { false };

    void processMetering(const float* const* inputData, int numChannels, int numSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};

