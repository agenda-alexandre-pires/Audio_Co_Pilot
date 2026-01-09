#include "SpreadingFunction.h"
#include "BarkAnalyzer.h"
#include <cmath>

namespace AudioCoPilot
{
    SpreadingFunction::SpreadingFunction()
    {
        calculateSpreadingMatrix();
    }
    
    void SpreadingFunction::calculateSpreadingMatrix()
    {
        for (int maskedBand = 0; maskedBand < NUM_BANDS; ++maskedBand)
        {
            for (int maskingBand = 0; maskingBand < NUM_BANDS; ++maskingBand)
            {
                if (maskedBand == maskingBand)
                {
                    // Mesma banda: sem atenuação
                    _spreadingMatrix[maskedBand][maskingBand] = 0.0f;
                }
                else
                {
                    // Calcula espalhamento para nível de referência (60 dB)
                    _spreadingMatrix[maskedBand][maskingBand] = 
                        calculateSpreading(maskingBand, maskedBand, 60.0f);
                }
            }
        }
    }
    
    float SpreadingFunction::calculateSpreading(int maskingBand, int maskedBand, float maskingLevel)
    {
        // Distância em Bark
        float dz = static_cast<float>(maskedBand - maskingBand);
        
        // Modelo de Schroeder modificado
        float slope = calculateSlope(dz, maskingLevel);
        
        // Atenuação baseada na distância e slope
        float attenuation = slope * std::abs(dz);
        
        // Limita a atenuação máxima
        return std::min(attenuation, 100.0f);
    }
    
    float SpreadingFunction::calculateSlope(float dz, float level)
    {
        if (dz < 0)
        {
            // Upward masking (frequências mais baixas mascaram mais altas)
            // Slope mais suave, mascaramento mais forte
            return 27.0f;  // dB/Bark
        }
        else
        {
            // Downward masking (frequências mais altas mascaram mais baixas)
            // Slope mais íngreme, dependente do nível
            // Quanto maior o nível, mais espalhamento para baixo
            return 24.0f + 0.23f * level - 0.2f * level;  // ~24-27 dB/Bark
        }
    }
    
    void SpreadingFunction::applySpreadingFunction(
        const std::array<float, NUM_BANDS>& inputLevels,
        std::array<float, NUM_BANDS>& outputLevels)
    {
        for (int maskedBand = 0; maskedBand < NUM_BANDS; ++maskedBand)
        {
            float maxMaskingLevel = -100.0f;
            
            for (int maskingBand = 0; maskingBand < NUM_BANDS; ++maskingBand)
            {
                // Nível efetivo do mascaramento
                float effectiveLevel = inputLevels[maskingBand] - 
                    _spreadingMatrix[maskedBand][maskingBand];
                
                maxMaskingLevel = std::max(maxMaskingLevel, effectiveLevel);
            }
            
            outputLevels[maskedBand] = maxMaskingLevel;
        }
    }
}