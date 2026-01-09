#pragma once

#include <JuceHeader.h>
#include <Accelerate/Accelerate.h>

namespace DSPUtils
{
    /**
     * Converte amplitude linear para dB
     */
    inline float linearToDb(float linear)
    {
        return (linear > 0.0f) ? 20.0f * std::log10(linear) : -100.0f;
    }

    /**
     * Converte dB para amplitude linear
     */
    inline float dbToLinear(float db)
    {
        return std::pow(10.0f, db / 20.0f);
    }

    /**
     * Calcula RMS de um buffer usando Accelerate (otimizado para Apple Silicon)
     */
    inline float calculateRMS(const float* data, int numSamples)
    {
        float sumSquares = 0.0f;
        vDSP_svesq(data, 1, &sumSquares, static_cast<vDSP_Length>(numSamples));
        return std::sqrt(sumSquares / static_cast<float>(numSamples));
    }

    /**
     * Encontra valor de pico absoluto usando Accelerate
     */
    inline float findPeak(const float* data, int numSamples)
    {
        float peak = 0.0f;
        vDSP_maxmgv(data, 1, &peak, static_cast<vDSP_Length>(numSamples));
        return peak;
    }

    /**
     * Aplica smoothing exponencial
     */
    inline float exponentialSmooth(float currentValue, float targetValue, float coefficient)
    {
        return coefficient * currentValue + (1.0f - coefficient) * targetValue;
    }
}

