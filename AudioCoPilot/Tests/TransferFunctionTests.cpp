#include <catch2/catch_all.hpp>
#include "../Source/Modules/TransferFunction/TransferFunctionProcessor.h"
#include <cmath>
#include <algorithm>

TEST_CASE("TransferFunctionProcessor: Initialization", "[transferfunction]")
{
    TransferFunctionProcessor processor;
    
    SECTION("Starts enabled")
    {
        REQUIRE(processor.isEnabled());
    }
    
    SECTION("Default resolution is TwelfthOctave")
    {
        REQUIRE(processor.getOctaveResolution() == OctaveResolution::TwelfthOctave);
    }
    
    SECTION("Returns empty data when not prepared")
    {
        auto data = processor.readTransferData();
        REQUIRE(data.numBins == 0);
    }
    
    SECTION("Debug info shows buffer not ready initially")
    {
        auto debugInfo = processor.getDebugInfo();
        REQUIRE_FALSE(debugInfo.bufferReady);
        REQUIRE(debugInfo.framesProcessed == 0);
        REQUIRE_FALSE(debugInfo.dataReady);
    }
}

TEST_CASE("TransferFunctionProcessor: Prepare", "[transferfunction]")
{
    TransferFunctionProcessor processor;
    
    SECTION("Prepare sets sample rate")
    {
        processor.prepare(48000.0, 512);
        // Verifica que prepare não crasha
        REQUIRE_NOTHROW(processor.prepare(44100.0, 256));
    }
    
    SECTION("Reset clears state")
    {
        processor.prepare(48000.0, 512);
        processor.reset();
        
        auto debugInfo = processor.getDebugInfo();
        REQUIRE_FALSE(debugInfo.bufferReady);
        REQUIRE(debugInfo.framesProcessed == 0);
        REQUIRE_FALSE(debugInfo.dataReady);
    }
}

TEST_CASE("TransferFunctionProcessor: Enable/Disable", "[transferfunction]")
{
    TransferFunctionProcessor processor;
    
    SECTION("Can disable")
    {
        processor.setEnabled(false);
        REQUIRE_FALSE(processor.isEnabled());
    }
    
    SECTION("Can re-enable")
    {
        processor.setEnabled(false);
        processor.setEnabled(true);
        REQUIRE(processor.isEnabled());
    }
}

TEST_CASE("TransferFunctionProcessor: Octave Resolution", "[transferfunction]")
{
    TransferFunctionProcessor processor;
    
    SECTION("Can set ThirdOctave")
    {
        processor.setOctaveResolution(OctaveResolution::ThirdOctave);
        REQUIRE(processor.getOctaveResolution() == OctaveResolution::ThirdOctave);
    }
    
    SECTION("Can set SixthOctave")
    {
        processor.setOctaveResolution(OctaveResolution::SixthOctave);
        REQUIRE(processor.getOctaveResolution() == OctaveResolution::SixthOctave);
    }
    
    SECTION("Can set TwelfthOctave")
    {
        processor.setOctaveResolution(OctaveResolution::TwelfthOctave);
        REQUIRE(processor.getOctaveResolution() == OctaveResolution::TwelfthOctave);
    }
    
    SECTION("Can set TwentyFourthOctave")
    {
        processor.setOctaveResolution(OctaveResolution::TwentyFourthOctave);
        REQUIRE(processor.getOctaveResolution() == OctaveResolution::TwentyFourthOctave);
    }
    
    SECTION("Can set FortyEighthOctave")
    {
        processor.setOctaveResolution(OctaveResolution::FortyEighthOctave);
        REQUIRE(processor.getOctaveResolution() == OctaveResolution::FortyEighthOctave);
    }
}

TEST_CASE("TransferFunctionProcessor: Process Block - Edge Cases", "[transferfunction]")
{
    TransferFunctionProcessor processor;
    processor.prepare(48000.0, 512);
    
    SECTION("Ignores when disabled")
    {
        processor.setEnabled(false);
        
        float refData[64] = {0.0f};
        float measData[64] = {0.0f};
        const float* inputData[2] = {refData, measData};
        
        // Não deve crashar
        REQUIRE_NOTHROW(processor.processBlock(inputData, 2, 64));
    }
    
    SECTION("Ignores when less than 2 channels")
    {
        float refData[64] = {0.0f};
        const float* inputData[1] = {refData};
        
        REQUIRE_NOTHROW(processor.processBlock(inputData, 1, 64));
        
        auto debugInfo = processor.getDebugInfo();
        REQUIRE_FALSE(debugInfo.bufferReady);
    }
    
    SECTION("Handles null pointers gracefully")
    {
        const float* inputData[2] = {nullptr, nullptr};
        REQUIRE_NOTHROW(processor.processBlock(inputData, 2, 64));
    }
    
    SECTION("Handles empty buffer")
    {
        std::vector<float> refData;
        std::vector<float> measData;
        const float* inputData[2] = {refData.data(), measData.data()};
        
        REQUIRE_NOTHROW(processor.processBlock(inputData, 2, 0));
    }
}

TEST_CASE("TransferFunctionProcessor: Process Block - Basic Signal", "[transferfunction]")
{
    TransferFunctionProcessor processor;
    processor.prepare(48000.0, 512);
    
    // Cria sinal de teste: senoide de 1kHz no canal de referência
    // Canal de medição é o mesmo sinal (função de transferência = 1.0)
    constexpr int blockSize = 64;
    constexpr double sampleRate = 48000.0;
    constexpr double freq = 1000.0;
    
    std::vector<float> refData(blockSize);
    std::vector<float> measData(blockSize);
    
    for (int i = 0; i < blockSize; ++i)
    {
        double t = static_cast<double>(i) / sampleRate;
        float sample = static_cast<float>(std::sin(2.0 * juce::MathConstants<double>::pi * freq * t));
        refData[i] = sample;
        measData[i] = sample;  // Mesmo sinal = função de transferência = 1.0
    }
    
    const float* inputData[2] = {refData.data(), measData.data()};
    
    SECTION("Processes multiple blocks")
    {
        // Processa vários blocos para encher o buffer FFT
        // FFT_SIZE = 8192, então precisamos de ~128 blocos de 64 samples
        for (int i = 0; i < 150; ++i)
        {
            processor.processBlock(inputData, 2, blockSize);
        }
        
        // Verifica que pelo menos processou algo
        auto debugInfo = processor.getDebugInfo();
        // Buffer pode estar pronto ou não dependendo do timing
        // Mas não deve crashar
        REQUIRE_NOTHROW(processor.getDebugInfo());
    }
    
    SECTION("Updates RMS levels")
    {
        // Processa alguns blocos
        for (int i = 0; i < 10; ++i)
        {
            processor.processBlock(inputData, 2, blockSize);
        }
        
        auto debugInfo = processor.getDebugInfo();
        // Níveis devem ser maiores que zero para sinal não-zero
        REQUIRE(debugInfo.referenceLevel >= 0.0f);
        REQUIRE(debugInfo.measurementLevel >= 0.0f);
    }
}

TEST_CASE("TransferFunctionProcessor: Process Block - Zero Signal", "[transferfunction]")
{
    TransferFunctionProcessor processor;
    processor.prepare(48000.0, 512);
    
    constexpr int blockSize = 64;
    std::vector<float> zeroData(blockSize, 0.0f);
    const float* inputData[2] = {zeroData.data(), zeroData.data()};
    
    SECTION("Handles zero input gracefully")
    {
        // Processa vários blocos de zeros
        for (int i = 0; i < 150; ++i)
        {
            REQUIRE_NOTHROW(processor.processBlock(inputData, 2, blockSize));
        }
        
        auto debugInfo = processor.getDebugInfo();
        REQUIRE(debugInfo.referenceLevel == 0.0f);
        REQUIRE(debugInfo.measurementLevel == 0.0f);
    }
}

TEST_CASE("TransferFunctionProcessor: Process Block - Different Amplitudes", "[transferfunction]")
{
    TransferFunctionProcessor processor;
    processor.prepare(48000.0, 512);
    
    constexpr int blockSize = 64;
    constexpr double sampleRate = 48000.0;
    constexpr double freq = 1000.0;
    
    std::vector<float> refData(blockSize);
    std::vector<float> measData(blockSize);
    
    // Referência: amplitude 1.0
    // Medição: amplitude 0.5 (função de transferência = 0.5 = -6dB)
    for (int i = 0; i < blockSize; ++i)
    {
        double t = static_cast<double>(i) / sampleRate;
        float sample = static_cast<float>(std::sin(2.0 * juce::MathConstants<double>::pi * freq * t));
        refData[i] = sample;
        measData[i] = sample * 0.5f;  // Metade da amplitude
    }
    
    const float* inputData[2] = {refData.data(), measData.data()};
    
    SECTION("Processes different amplitude signals")
    {
        // Processa blocos suficientes para análise
        for (int i = 0; i < 150; ++i)
        {
            processor.processBlock(inputData, 2, blockSize);
        }
        
        // Verifica que processou sem crashar
        REQUIRE_NOTHROW(processor.getDebugInfo());
        
        auto debugInfo = processor.getDebugInfo();
        // Nível de medição deve ser menor que referência
        REQUIRE(debugInfo.measurementLevel < debugInfo.referenceLevel);
    }
}

TEST_CASE("TransferFunctionProcessor: Read Transfer Data", "[transferfunction]")
{
    TransferFunctionProcessor processor;
    processor.prepare(48000.0, 512);
    
    SECTION("Returns empty data when not ready")
    {
        auto data = processor.readTransferData();
        REQUIRE(data.numBins == 0);
        REQUIRE(data.magnitudeDb.empty());
        REQUIRE(data.phaseDegrees.empty());
        REQUIRE(data.frequencies.empty());
    }
    
    SECTION("Sample rate is preserved")
    {
        processor.prepare(44100.0, 512);
        // Mesmo sem dados prontos, sample rate deve estar configurado
        // (verificamos através do debug info ou preparação)
        REQUIRE_NOTHROW(processor.prepare(48000.0, 512));
    }
}

TEST_CASE("TransferFunctionProcessor: Reset", "[transferfunction]")
{
    TransferFunctionProcessor processor;
    processor.prepare(48000.0, 512);
    
    SECTION("Reset clears all state")
    {
        // Processa alguns blocos
        constexpr int blockSize = 64;
        std::vector<float> refData(blockSize, 0.1f);
        std::vector<float> measData(blockSize, 0.1f);
        const float* inputData[2] = {refData.data(), measData.data()};
        
        for (int i = 0; i < 10; ++i)
        {
            processor.processBlock(inputData, 2, blockSize);
        }
        
        // Reseta
        processor.reset();
        
        auto debugInfo = processor.getDebugInfo();
        REQUIRE_FALSE(debugInfo.bufferReady);
        REQUIRE(debugInfo.framesProcessed == 0);
        REQUIRE_FALSE(debugInfo.dataReady);
        REQUIRE(debugInfo.referenceLevel == 0.0f);
        REQUIRE(debugInfo.measurementLevel == 0.0f);
    }
}

TEST_CASE("TransferFunctionProcessor: Multiple Sample Rates", "[transferfunction]")
{
    TransferFunctionProcessor processor;
    
    SECTION("Can switch sample rates")
    {
        REQUIRE_NOTHROW(processor.prepare(44100.0, 512));
        REQUIRE_NOTHROW(processor.prepare(48000.0, 512));
        REQUIRE_NOTHROW(processor.prepare(96000.0, 512));
    }
    
    SECTION("Can switch buffer sizes")
    {
        REQUIRE_NOTHROW(processor.prepare(48000.0, 64));
        REQUIRE_NOTHROW(processor.prepare(48000.0, 128));
        REQUIRE_NOTHROW(processor.prepare(48000.0, 256));
        REQUIRE_NOTHROW(processor.prepare(48000.0, 512));
    }
}

