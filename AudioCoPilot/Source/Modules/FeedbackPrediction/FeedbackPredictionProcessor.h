#pragma once

#include <juce_core/juce_core.h>
#include "HowlingDetector.h"
#include <Accelerate/Accelerate.h>
#include <atomic>
#include <vector>

namespace AudioCoPilot {

class FeedbackPredictionProcessor {
public:
    FeedbackPredictionProcessor();
    ~FeedbackPredictionProcessor();
    
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Novo método que suporta dois canais (Reference vs Measurement)
    void process(const float* measurementBuffer, const float* referenceBuffer, int numSamples);
    
    // Mantém compatibilidade (assume reference = null)
    void process(const float* inputBuffer, int numSamples);
    
    // Acesso aos resultados
    const HowlingDetector& getDetector() const { return _detector; }
    
    // Controles
    void setEnabled(bool enabled) { _enabled.store(enabled); }
    bool isEnabled() const { return _enabled.load(); }
    
    // Learn Stage
    void startLearning();
    void stopLearning();
    bool isLearning() const { return _learning.load(); }
    void clearLearned();
    
private:
    // FFT setup
    FFTSetup _fftSetup = nullptr;
    int _fftOrder = 12; // 2^12 = 4096
    int _fftSize = 4096;
    int _hopSize = 1024;
    
    // Buffers para Measurement (Mic)
    std::vector<float> _inputBufferMeas;
    std::vector<float> _windowedBufferMeas;
    std::vector<float> _fftRealMeas;
    std::vector<float> _fftImagMeas;
    std::vector<float> _magnitudeSpectrumMeas;
    DSPSplitComplex _splitComplexMeas;
    
    // Buffers para Reference (Console)
    std::vector<float> _inputBufferRef;
    std::vector<float> _windowedBufferRef;
    std::vector<float> _fftRealRef;
    std::vector<float> _fftImagRef;
    std::vector<float> _magnitudeSpectrumRef;
    DSPSplitComplex _splitComplexRef;
    
    // Buffer temporário para conversão FFT (correção de casting)
    std::vector<DSPComplex> _fftTempBuffer;
    
    // Buffer temporário para sqrt (vvsqrtf não suporta in-place)
    std::vector<float> _sqrtTempBuffer;
    
    // Windowing
    std::vector<float> _hannWindow;
    
    // Estado
    int _inputWritePos = 0;
    double _sampleRate = 48000.0;
    std::atomic<bool> _enabled { false };
    std::atomic<bool> _learning { false };
    
    // Detector
    HowlingDetector _detector;
    
    // Learn Stage
    std::vector<float> _learnedFrequencies;
    std::vector<int> _frequencyHitCount;
    int _learnFrameCount = 0;
    static constexpr int LEARN_FRAMES = 500;
    
    void processFFT(bool hasReference);
    void createHannWindow();
    void processLearnFrame();
    
    // Helper para processar FFT de um canal
    void performSingleChannelFFT(const std::vector<float>& windowed, 
                               DSPSplitComplex& splitComplex,
                               std::vector<float>& magnitude);
};

} // namespace AudioCoPilot
