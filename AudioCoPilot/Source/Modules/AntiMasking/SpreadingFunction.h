#pragma once

#include <juce_core/juce_core.h>
#include <array>

namespace AudioCoPilot
{
    /**
     * @brief Implementa a função de espalhamento (spreading function) para mascaramento
     * 
     * A spreading function modela como a energia de uma banda crítica se espalha
     * para bandas vizinhas, causando mascaramento. Baseado no modelo de Schroeder (1979).
     */
    class SpreadingFunction
    {
    public:
        static constexpr int NUM_BANDS = 24;
        
        SpreadingFunction();
        ~SpreadingFunction() = default;
        
        /**
         * @brief Calcula a matriz de espalhamento
         * 
         * A matriz spreading[i][j] contém a atenuação (em dB) que a banda j
         * causa na banda i devido ao mascaramento.
         */
        void calculateSpreadingMatrix();
        
        /**
         * @brief Aplica a função de espalhamento a um espectro Bark
         * @param inputLevels Níveis de entrada em cada banda (dB)
         * @param outputLevels Níveis de saída com espalhamento aplicado (dB)
         */
        void applySpreadingFunction(const std::array<float, NUM_BANDS>& inputLevels,
                                    std::array<float, NUM_BANDS>& outputLevels);
        
        /**
         * @brief Calcula o espalhamento de uma banda para outra
         * @param maskingBand Banda que causa o mascaramento
         * @param maskedBand Banda que sofre o mascaramento
         * @param maskingLevel Nível da banda mascaradora (dB)
         * @return Atenuação em dB
         */
        float calculateSpreading(int maskingBand, int maskedBand, float maskingLevel);
        
        /**
         * @brief Retorna a matriz de espalhamento
         */
        const std::array<std::array<float, NUM_BANDS>, NUM_BANDS>& getSpreadingMatrix() const
        {
            return _spreadingMatrix;
        }
        
    private:
        // Matriz de espalhamento [banda mascarada][banda mascaradora]
        std::array<std::array<float, NUM_BANDS>, NUM_BANDS> _spreadingMatrix;
        
        /**
         * @brief Calcula a slope de espalhamento baseada na distância em Bark
         * @param dz Distância em Bark (negativo = abaixo, positivo = acima)
         * @param level Nível do mascarador em dB
         * @return Atenuação em dB/Bark
         */
        float calculateSlope(float dz, float level);
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpreadingFunction)
    };
}