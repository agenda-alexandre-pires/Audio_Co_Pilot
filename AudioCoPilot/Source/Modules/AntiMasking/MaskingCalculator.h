#pragma once

#include <juce_core/juce_core.h>
#include "BarkAnalyzer.h"
#include "SpreadingFunction.h"
#include <array>

namespace AudioCoPilot
{
    /**
     * @brief Estrutura para armazenar resultados de mascaramento por banda
     */
    struct BandMaskingResult
    {
        float targetLevel = -100.0f;           // Nível do target na banda (dB)
        float totalMaskingLevel = -100.0f;     // Nível total de mascaramento (dB)
        float signalToMaskRatio = 0.0f;        // Razão sinal/máscara (dB)
        float audibility = 1.0f;               // 0.0 = completamente mascarado, 1.0 = claramente audível
        int dominantMasker = -1;               // Índice do masker dominante (-1 se nenhum)
        std::array<float, 3> maskerContributions = {0.0f, 0.0f, 0.0f}; // Contribuição de cada masker
    };
    
    /**
     * @brief Estrutura para resultado geral de mascaramento
     */
    struct MaskingAnalysisResult
    {
        std::array<BandMaskingResult, 24> bandResults;
        float overallAudibility = 1.0f;        // Audibilidade média ponderada
        float criticalBandCount = 0;           // Número de bandas com mascaramento crítico
        std::array<float, 24> targetSpectrum;  // Espectro do target
        std::array<float, 24> combinedMaskingThreshold; // Limiar de mascaramento combinado
    };
    
    /**
     * @brief Calcula mascaramento entre 1 target e até 3 maskers
     */
    class MaskingCalculator
    {
    public:
        static constexpr int NUM_BANDS = 24;
        static constexpr int MAX_MASKERS = 3;
        static constexpr float MASKING_THRESHOLD_OFFSET = 5.5f; // dB abaixo do masker
        
        MaskingCalculator();
        ~MaskingCalculator() = default;
        
        /**
         * @brief Prepara o calculador
         * @param sampleRate Taxa de amostragem
         */
        void prepare(double sampleRate);
        
        /**
         * @brief Define os espectros dos maskers
         * @param maskerIndex Índice do masker (0-2)
         * @param spectrum Espectro Bark em dB
         */
        void setMaskerSpectrum(int maskerIndex, const std::array<float, NUM_BANDS>& spectrum);
        
        /**
         * @brief Define o espectro do target
         * @param spectrum Espectro Bark em dB
         */
        void setTargetSpectrum(const std::array<float, NUM_BANDS>& spectrum);
        
        /**
         * @brief Habilita/desabilita um masker
         * @param maskerIndex Índice do masker (0-2)
         * @param enabled Estado
         */
        void setMaskerEnabled(int maskerIndex, bool enabled);
        
        /**
         * @brief Calcula o mascaramento
         * @return Resultado da análise
         */
        MaskingAnalysisResult calculate();
        
        /**
         * @brief Atualiza a média de 5 segundos
         * @param newResult Novo resultado para adicionar à média
         */
        void updateRunningAverage(const MaskingAnalysisResult& newResult);
        
        /**
         * @brief Retorna a média atual de 5 segundos
         */
        const MaskingAnalysisResult& getAveragedResult() const { return _averagedResult; }
        
        /**
         * @brief Reseta a média
         */
        void resetAverage();
        
    private:
        double _sampleRate = 48000.0;
        
        SpreadingFunction _spreadingFunction;
        
        // Espectros de entrada
        std::array<float, NUM_BANDS> _targetSpectrum;
        std::array<std::array<float, NUM_BANDS>, MAX_MASKERS> _maskerSpectrums;
        std::array<bool, MAX_MASKERS> _maskerEnabled = {false, false, false};
        
        // Espectros com spreading aplicado
        std::array<float, NUM_BANDS> _targetWithSpreading;
        std::array<std::array<float, NUM_BANDS>, MAX_MASKERS> _maskersWithSpreading;
        
        // Buffer circular para média de 5 segundos
        static constexpr int AVERAGE_BUFFER_SIZE = 50; // ~5 segundos @ 10 Hz update
        std::array<MaskingAnalysisResult, AVERAGE_BUFFER_SIZE> _resultBuffer;
        int _bufferIndex = 0;
        int _bufferCount = 0;
        
        // Resultado médio
        MaskingAnalysisResult _averagedResult;
        
        void calculateCombinedMaskingThreshold(std::array<float, NUM_BANDS>& threshold);
        float calculateAudibility(float targetLevel, float maskingThreshold);
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MaskingCalculator)
    };
}