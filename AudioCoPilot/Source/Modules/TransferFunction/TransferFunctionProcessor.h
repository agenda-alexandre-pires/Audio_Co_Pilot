#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>
#include <complex>
#include <vector>

/**
 * TransferFunctionProcessor
 * 
 * Calcula a função de transferência H(f) = Y(f) / X(f) entre dois canais:
 * - Canal 0: Referência (X)
 * - Canal 1: Medição (Y)
 * 
 * Thread-safe: processamento no audio thread, leitura na UI thread
 */
enum class OctaveResolution
{
    ThirdOctave = 3,   // 1/3 de oitava
    SixthOctave = 6,   // 1/6 de oitava
    TwelfthOctave = 12, // 1/12 de oitava
    TwentyFourthOctave = 24, // 1/24 de oitava
    FortyEighthOctave = 48   // 1/48 de oitava
};

class TransferFunctionProcessor
{
public:
    TransferFunctionProcessor();
    ~TransferFunctionProcessor() = default;

    /**
     * Prepara o processador para um novo sample rate
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * Processa um bloco de áudio (chamado no audio thread)
     * @param inputData Array de canais de entrada [reference, measurement]
     * @param numChannels Número de canais (precisa ser >= 2)
     * @param numSamples Tamanho do bloco
     */
    void processBlock(const float* const* inputData, int numChannels, int numSamples);

    /**
     * Estrutura para armazenar dados de transferência
     */
    struct TransferData
    {
        std::vector<float> magnitudeDb;  // Magnitude em dB por bin de frequência
        std::vector<float> phaseDegrees;  // Fase em graus por bin
        std::vector<float> frequencies;   // Frequências correspondentes (Hz)
        int numBins = 0;
        double sampleRate = 48000.0;
    };

    /**
     * Lê os dados de transferência calculados (thread-safe)
     * Retorna dados válidos apenas se ambos os canais têm sinal suficiente
     */
    TransferData readTransferData();
    
    /**
     * Debug: retorna informações sobre o estado atual
     */
    struct DebugInfo
    {
        bool bufferReady = false;
        int framesProcessed = 0;
        bool dataReady = false;
        float referenceLevel = 0.0f;  // RMS do canal de referência
        float measurementLevel = 0.0f;  // RMS do canal de medição
    };
    DebugInfo getDebugInfo() const;

    /**
     * Reseta a análise
     */
    void reset();

    /**
     * Define se a análise está ativa
     */
    void setEnabled(bool enabled) { _enabled = enabled; }
    bool isEnabled() const { return _enabled.load(); }
    
    /**
     * Define a resolução de banda (1/3, 1/6, 1/12, 1/24, 1/48 de oitava)
     */
    void setOctaveResolution(OctaveResolution resolution);
    OctaveResolution getOctaveResolution() const { return _octaveResolution.load(); }

private:
    static constexpr int FFT_ORDER = 13;  // 2^13 = 8192 pontos
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;
    static constexpr int NUM_BINS = FFT_SIZE / 2 + 1;

    std::atomic<bool> _enabled { true };
    std::atomic<double> _sampleRate { 48000.0 };
    std::atomic<OctaveResolution> _octaveResolution { OctaveResolution::TwelfthOctave };

    // FFT engines para cada canal
    juce::dsp::FFT _fft { FFT_ORDER };
    
    // Buffers circulares para acumular dados
    std::array<float, FFT_SIZE> _referenceBuffer;
    std::array<float, FFT_SIZE> _measurementBuffer;
    int _bufferWritePos = 0;
    bool _bufferReady = false;

    // Espectros complexos (frequência) - temporários para cada frame
    std::array<std::complex<float>, NUM_BINS> _referenceSpectrum;
    std::array<std::complex<float>, NUM_BINS> _measurementSpectrum;
    
    // Função de transferência acumulada para média
    std::array<std::complex<float>, NUM_BINS> _transferFunctionAccum;
    std::array<std::complex<float>, NUM_BINS> _transferFunction;

    // Dados de saída (lock-free, atualizados periodicamente)
    // Usa array maior para acomodar diferentes resoluções de banda
    // 1/48 de oitava de 20Hz a 20kHz gera ~478 bandas
    static constexpr int MAX_BANDS = 500;
    std::array<std::atomic<float>, MAX_BANDS> _magnitudeDb;
    std::array<std::atomic<float>, MAX_BANDS> _phaseDegrees;
    std::atomic<bool> _dataReady { false };

    // Windowing
    juce::dsp::WindowingFunction<float> _window;

    // Buffers temporários para FFT (evita alocação na stack)
    std::array<float, FFT_SIZE> _refWindowedBuffer;
    std::array<float, FFT_SIZE> _measWindowedBuffer;
    std::array<std::complex<float>, FFT_SIZE> _refComplexBuffer;
    std::array<std::complex<float>, FFT_SIZE> _measComplexBuffer;
    std::array<std::complex<float>, FFT_SIZE> _refOutputBuffer;
    std::array<std::complex<float>, FFT_SIZE> _measOutputBuffer;

    // Contador de frames processados (para averaging)
    int _framesProcessed = 0;
    static constexpr int FRAMES_TO_AVERAGE = 8;  // Média de 8 frames para estabilidade
    
    // Debug: níveis RMS dos canais
    std::atomic<float> _referenceLevel { 0.0f };
    std::atomic<float> _measurementLevel { 0.0f };

    void performFFT();
    void calculateTransferFunction();
    void updateOutputData();
    
    /**
     * Calcula bandas de oitava a partir dos bins da FFT
     */
    void calculateOctaveBands(const std::array<std::complex<float>, NUM_BINS>& spectrum,
                               std::vector<float>& bandMagnitudes,
                               std::vector<float>& bandPhases,
                               std::vector<float>& bandFrequencies);
    
    /**
     * Calcula frequências centrais das bandas de oitava
     */
    std::vector<float> calculateOctaveBandFrequencies(double sampleRate, OctaveResolution resolution);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransferFunctionProcessor)
};

