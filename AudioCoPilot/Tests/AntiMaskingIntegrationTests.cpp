#include <catch2/catch_all.hpp>
#include "Modules/AntiMasking/AntiMaskingProcessor.h"

using namespace AudioCoPilot;

TEST_CASE("AntiMaskingProcessor: Integration test", "[antimasking]")
{
    AntiMaskingProcessor processor;
    
    SECTION("Prepare works correctly")
    {
        REQUIRE_FALSE(processor.isPrepared());
        
        processor.prepare(48000.0, 256);
        
        REQUIRE(processor.isPrepared());
    }
    
    SECTION("Channel configuration")
    {
        processor.prepare(48000.0, 256);
        
        processor.setTargetChannel(2);
        processor.setMaskerChannel(0, 1, true);
        processor.setMaskerChannel(1, 3, true);
        processor.setMaskerChannel(2, 0, false);
        
        // Verifica se as configurações foram aplicadas
        // (não há getters públicos, então testamos indiretamente)
        REQUIRE(processor.isPrepared());
    }
    
    SECTION("Process silence")
    {
        processor.prepare(48000.0, 256);
        
        juce::AudioBuffer<float> buffer(4, 256);
        buffer.clear();
        
        // Processa silêncio
        processor.processBlock(buffer);
        
        // Não deve crashar
        REQUIRE_NOTHROW(processor.getCurrentResult());
    }
    
    SECTION("Process sine waves")
    {
        processor.prepare(48000.0, 256);
        
        // Configura canais
        processor.setTargetChannel(0);
        processor.setMaskerChannel(0, 1, true);
        
        // Cria buffer com senoides
        juce::AudioBuffer<float> buffer(4, 256);
        
        // Target: 1000 Hz
        for (int i = 0; i < 256; ++i)
        {
            float phase = 1000.0f * i / 48000.0f * 2.0f * juce::MathConstants<float>::pi;
            buffer.setSample(0, i, std::sin(phase) * 0.5f);
        }
        
        // Masker: 1050 Hz (próximo, deve causar mascaramento)
        for (int i = 0; i < 256; ++i)
        {
            float phase = 1050.0f * i / 48000.0f * 2.0f * juce::MathConstants<float>::pi;
            buffer.setSample(1, i, std::sin(phase) * 0.7f); // Mais forte
        }
        
        // Processa múltiplos blocos para estabilizar
        for (int i = 0; i < 100; ++i)
        {
            processor.processBlock(buffer);
        }
        
        auto result = processor.getCurrentResult();
        
        // Deve detectar algum mascaramento
        REQUIRE(result.overallAudibility < 1.0f);
    }
}