#include "MaskingCalculator.h"
#include <cmath>
#include <numeric>

namespace AudioCoPilot
{
    MaskingCalculator::MaskingCalculator()
    {
        _targetSpectrum.fill(-100.0f);
        for (auto& spectrum : _maskerSpectrums)
            spectrum.fill(-100.0f);
        
        _targetWithSpreading.fill(-100.0f);
        for (auto& spectrum : _maskersWithSpreading)
            spectrum.fill(-100.0f);
        
        resetAverage();
    }
    
    void MaskingCalculator::prepare(double sampleRate)
    {
        _sampleRate = sampleRate;
        resetAverage();
    }
    
    void MaskingCalculator::setMaskerSpectrum(int maskerIndex, 
                                              const std::array<float, NUM_BANDS>& spectrum)
    {
        if (maskerIndex >= 0 && maskerIndex < MAX_MASKERS)
        {
            _maskerSpectrums[maskerIndex] = spectrum;
            _spreadingFunction.applySpreadingFunction(spectrum, 
                                                       _maskersWithSpreading[maskerIndex]);
        }
    }
    
    void MaskingCalculator::setTargetSpectrum(const std::array<float, NUM_BANDS>& spectrum)
    {
        _targetSpectrum = spectrum;
        _spreadingFunction.applySpreadingFunction(spectrum, _targetWithSpreading);
    }
    
    void MaskingCalculator::setMaskerEnabled(int maskerIndex, bool enabled)
    {
        if (maskerIndex >= 0 && maskerIndex < MAX_MASKERS)
            _maskerEnabled[maskerIndex] = enabled;
    }
    
    MaskingAnalysisResult MaskingCalculator::calculate()
    {
        MaskingAnalysisResult result;
        result.targetSpectrum = _targetSpectrum;
        
        // Calcula o limiar de mascaramento combinado
        calculateCombinedMaskingThreshold(result.combinedMaskingThreshold);
        
        float totalAudibility = 0.0f;
        int criticalBands = 0;
        
        for (int band = 0; band < NUM_BANDS; ++band)
        {
            auto& bandResult = result.bandResults[band];
            bandResult.targetLevel = _targetSpectrum[band];
            bandResult.totalMaskingLevel = result.combinedMaskingThreshold[band];
            
            // Razão sinal/máscara
            bandResult.signalToMaskRatio = bandResult.targetLevel - bandResult.totalMaskingLevel;
            
            // Calcula audibilidade
            bandResult.audibility = calculateAudibility(bandResult.targetLevel, 
                                                        bandResult.totalMaskingLevel);
            
            // Encontra masker dominante
            float maxMaskerLevel = -100.0f;
            for (int m = 0; m < MAX_MASKERS; ++m)
            {
                if (_maskerEnabled[m])
                {
                    float contribution = _maskersWithSpreading[m][band] - MASKING_THRESHOLD_OFFSET;
                    bandResult.maskerContributions[m] = contribution;
                    
                    if (contribution > maxMaskerLevel)
                    {
                        maxMaskerLevel = contribution;
                        bandResult.dominantMasker = m;
                    }
                }
            }
            
            totalAudibility += bandResult.audibility;
            
            if (bandResult.audibility < 0.5f)
                ++criticalBands;
        }
        
        result.overallAudibility = totalAudibility / NUM_BANDS;
        result.criticalBandCount = static_cast<float>(criticalBands);
        
        return result;
    }
    
    void MaskingCalculator::calculateCombinedMaskingThreshold(std::array<float, NUM_BANDS>& threshold)
    {
        for (int band = 0; band < NUM_BANDS; ++band)
        {
            // Combina maskers usando soma de potência
            float combinedPower = 0.0f;
            
            for (int m = 0; m < MAX_MASKERS; ++m)
            {
                if (_maskerEnabled[m])
                {
                    float level = _maskersWithSpreading[m][band] - MASKING_THRESHOLD_OFFSET;
                    combinedPower += std::pow(10.0f, level / 10.0f);
                }
            }
            
            if (combinedPower > 0.0f)
                threshold[band] = 10.0f * std::log10(combinedPower);
            else
                threshold[band] = -100.0f;
        }
    }
    
    float MaskingCalculator::calculateAudibility(float targetLevel, float maskingThreshold)
    {
        // Se o target está bem abaixo do threshold, não é audível
        if (targetLevel < -90.0f)
            return 0.0f;
        
        // Diferença em dB
        float diff = targetLevel - maskingThreshold;
        
        // Mapeia para 0-1:
        // diff < -10 dB: audibilidade = 0 (completamente mascarado)
        // diff > +10 dB: audibilidade = 1 (claramente audível)
        // Entre -10 e +10: transição suave
        float audibility = (diff + 10.0f) / 20.0f;
        return juce::jlimit(0.0f, 1.0f, audibility);
    }
    
    void MaskingCalculator::updateRunningAverage(const MaskingAnalysisResult& newResult)
    {
        _resultBuffer[_bufferIndex] = newResult;
        _bufferIndex = (_bufferIndex + 1) % AVERAGE_BUFFER_SIZE;
        _bufferCount = std::min(_bufferCount + 1, AVERAGE_BUFFER_SIZE);
        
        // Calcula média
        _averagedResult = MaskingAnalysisResult();
        
        for (int i = 0; i < _bufferCount; ++i)
        {
            const auto& sample = _resultBuffer[i];
            _averagedResult.overallAudibility += sample.overallAudibility;
            _averagedResult.criticalBandCount += sample.criticalBandCount;
            
            for (int band = 0; band < NUM_BANDS; ++band)
            {
                _averagedResult.targetSpectrum[band] += sample.targetSpectrum[band];
                _averagedResult.combinedMaskingThreshold[band] += 
                    sample.combinedMaskingThreshold[band];
                _averagedResult.bandResults[band].audibility += 
                    sample.bandResults[band].audibility;
                _averagedResult.bandResults[band].signalToMaskRatio += 
                    sample.bandResults[band].signalToMaskRatio;
            }
        }
        
        float invCount = 1.0f / _bufferCount;
        _averagedResult.overallAudibility *= invCount;
        _averagedResult.criticalBandCount *= invCount;
        
        for (int band = 0; band < NUM_BANDS; ++band)
        {
            _averagedResult.targetSpectrum[band] *= invCount;
            _averagedResult.combinedMaskingThreshold[band] *= invCount;
            _averagedResult.bandResults[band].audibility *= invCount;
            _averagedResult.bandResults[band].signalToMaskRatio *= invCount;
        }
    }
    
    void MaskingCalculator::resetAverage()
    {
        _bufferIndex = 0;
        _bufferCount = 0;
        _averagedResult = MaskingAnalysisResult();
    }
}