#include <catch2/catch_all.hpp>
#include "Modules/AntiMasking/MaskingCalculator.h"

using namespace AudioCoPilot;

TEST_CASE("MaskingCalculator: Basic operation", "[masking]")
{
    MaskingCalculator calculator;
    calculator.prepare(48000.0);
    
    SECTION("No maskers gives full audibility")
    {
        std::array<float, 24> targetSpectrum;
        targetSpectrum.fill(-20.0f); // Sinal a -20 dB
        
        calculator.setTargetSpectrum(targetSpectrum);
        // Nenhum masker habilitado
        
        auto result = calculator.calculate();
        
        REQUIRE(result.overallAudibility > 0.9f);
    }
    
    SECTION("Strong masker reduces audibility")
    {
        std::array<float, 24> targetSpectrum;
        std::array<float, 24> maskerSpectrum;
        
        targetSpectrum.fill(-30.0f); // Target fraco
        maskerSpectrum.fill(-10.0f); // Masker forte
        
        calculator.setTargetSpectrum(targetSpectrum);
        calculator.setMaskerSpectrum(0, maskerSpectrum);
        calculator.setMaskerEnabled(0, true);
        
        auto result = calculator.calculate();
        
        REQUIRE(result.overallAudibility < 0.5f);
    }
    
    SECTION("Target louder than masker remains audible")
    {
        std::array<float, 24> targetSpectrum;
        std::array<float, 24> maskerSpectrum;
        
        targetSpectrum.fill(-10.0f); // Target forte
        maskerSpectrum.fill(-40.0f); // Masker fraco
        
        calculator.setTargetSpectrum(targetSpectrum);
        calculator.setMaskerSpectrum(0, maskerSpectrum);
        calculator.setMaskerEnabled(0, true);
        
        auto result = calculator.calculate();
        
        REQUIRE(result.overallAudibility > 0.8f);
    }
}

TEST_CASE("MaskingCalculator: Running average", "[masking]")
{
    MaskingCalculator calculator;
    calculator.prepare(48000.0);
    
    SECTION("Average stabilizes over time")
    {
        std::array<float, 24> targetSpectrum;
        targetSpectrum.fill(-20.0f);
        
        calculator.setTargetSpectrum(targetSpectrum);
        
        // Processa múltiplos frames
        for (int i = 0; i < 100; ++i)
        {
            auto result = calculator.calculate();
            calculator.updateRunningAverage(result);
        }
        
        auto averaged = calculator.getAveragedResult();
        
        // Deve estar próximo do valor instantâneo após estabilizar
        REQUIRE(averaged.overallAudibility > 0.8f);
    }
}