#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>
#include <array>
#include <vector>

namespace AudioCoPilot
{
    /**
     * @brief Analisa sinais de áudio e converte para representação em Bark Scale
     * 
     * A Bark Scale divide o espectro audível em 24 bandas críticas que correspondem
     * à resolução de frequência da cóclea humana. Esta classe realiza FFT no sinal
     * de entrada e agrupa as bins em bandas Bark para análise psicoacústica.
     */
    class BarkAnalyzer
    {
    public:
        static constexpr int NUM_BARK_BANDS = 24;
        static constexpr int FFT_ORDER = 12;  // 4096 samples
        static constexpr int FFT_SIZE = 1 << FFT_ORDER;
        
        BarkAnalyzer();
        ~BarkAnalyzer() = default;
        
        /**
         * @brief Prepara o analisador para uma nova sample rate
         * @param sampleRate Taxa de amostragem em Hz
         */
        void prepare(double sampleRate);
        
        /**
         * @brief Processa um bloco de amostras e atualiza o espectro Bark
         * @param samples Ponteiro para as amostras
         * @param numSamples Número de amostras no bloco
         */
        void processBlock(const float* samples, int numSamples);
        
        /**
         * @brief Retorna os níveis atuais em cada banda Bark (em dB)
         * @return Array com 24 valores em dB
         */
        const std::array<float, NUM_BARK_BANDS>& getBarkLevels() const { return _barkLevelsDB; }
        
        /**
         * @brief Retorna os níveis suavizados (para display)
         * @return Array com 24 valores em dB suavizados
         */
        const std::array<float, NUM_BARK_BANDS>& getSmoothedBarkLevels() const { return _smoothedBarkLevels; }
        
        /**
         * @brief Converte frequência em Hz para índice de banda Bark
         * @param hz Frequência em Hz
         * @return Índice da banda Bark (0-23)
         */
        static int hzToBarkBand(float hz);
        
        /**
         * @brief Converte frequência em Hz para valor Bark contínuo
         * @param hz Frequência em Hz
         * @return Valor em Bark (0-24)
         */
        static float hzToBark(float hz);
        
        /**
         * @brief Converte valor Bark para frequência em Hz
         * @param bark Valor em Bark
         * @return Frequência em Hz
         */
        static float barkToHz(float bark);
        
        /**
         * @brief Retorna a frequência central de uma banda Bark
         * @param band Índice da banda (0-23)
         * @return Frequência central em Hz
         */
        static float getBandCenterHz(int band);
        
        /**
         * @brief Retorna as bordas de frequência de uma banda Bark
         * @param band Índice da banda (0-23)
         * @return Par com frequência inferior e superior em Hz
         */
        static std::pair<float, float> getBandEdgesHz(int band);
        
        // Bordas das bandas críticas em Hz
        static const std::array<float, 25> BARK_EDGES;
        
        // Centros das bandas críticas em Hz
        static const std::array<float, 24> BARK_CENTERS;
        
    private:
        double _sampleRate = 48000.0;
        
        // FFT
        std::unique_ptr<juce::dsp::FFT> _fft;
        std::unique_ptr<juce::dsp::WindowingFunction<float>> _window;
        
        // Buffers
        std::array<float, FFT_SIZE> _fftInput;
        std::array<float, FFT_SIZE * 2> _fftOutput;
        std::array<float, FFT_SIZE> _inputBuffer;
        int _inputBufferIndex = 0;
        
        // Mapeamento de bins FFT para bandas Bark
        std::array<int, NUM_BARK_BANDS> _bandStartBin;
        std::array<int, NUM_BARK_BANDS> _bandEndBin;
        
        // Resultados
        std::array<float, NUM_BARK_BANDS> _barkLevelsDB;
        std::array<float, NUM_BARK_BANDS> _smoothedBarkLevels;
        
        // Smoothing
        float _smoothingCoeff = 0.0f;
        
        void calculateBinMapping();
        void performFFT();
        void calculateBarkLevels();
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BarkAnalyzer)
    };
}