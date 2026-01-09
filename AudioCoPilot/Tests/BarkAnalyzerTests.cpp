#include <catch2/catch_all.hpp>
#include "Modules/AntiMasking/BarkAnalyzer.h"

using namespace AudioCoPilot;

TEST_CASE("BarkAnalyzer: Hz to Bark conversion", "[bark]")
{
    SECTION("Known frequency values")
    {
        // 1000 Hz é aproximadamente 8.5 Bark
        float bark1000 = BarkAnalyzer::hzToBark(1000.0f);
        REQUIRE(bark1000 > 8.0f);
        REQUIRE(bark1000 < 9.0f);
        
        // 100 Hz é aproximadamente 1 Bark
        float bark100 = BarkAnalyzer::hzToBark(100.0f);
        REQUIRE(bark100 > 0.5f);
        REQUIRE(bark100 < 1.5f);
    }
    
    SECTION("Inverse conversion")
    {
        // Testa ida e volta
        float originalHz = 2000.0f;
        float bark = BarkAnalyzer::hzToBark(originalHz);
        float recoveredHz = BarkAnalyzer::barkToHz(bark);
        
        REQUIRE(recoveredHz == Catch::Approx(originalHz).margin(1.0f));
    }
}

TEST_CASE("BarkAnalyzer: Band mapping", "[bark]")
{
    SECTION("Band edges are monotonic")
    {
        for (int i = 0; i < 24; ++i)
        {
            REQUIRE(BarkAnalyzer::BARK_EDGES[i] < BarkAnalyzer::BARK_EDGES[i + 1]);
        }
    }
    
    SECTION("Band centers are within edges")
    {
        for (int i = 0; i < 24; ++i)
        {
            float center = BarkAnalyzer::BARK_CENTERS[i];
            REQUIRE(center >= BarkAnalyzer::BARK_EDGES[i]);
            REQUIRE(center <= BarkAnalyzer::BARK_EDGES[i + 1]);
        }
    }
    
    SECTION("Hz to band index")
    {
        REQUIRE(BarkAnalyzer::hzToBarkBand(50.0f) == 0);
        REQUIRE(BarkAnalyzer::hzToBarkBand(1000.0f) == 8);
        REQUIRE(BarkAnalyzer::hzToBarkBand(14000.0f) == 23);
    }
}

TEST_CASE("BarkAnalyzer: Processing", "[bark]")
{
    BarkAnalyzer analyzer;
    analyzer.prepare(48000.0);
    
    SECTION("Silent input gives low levels")
    {
        std::array<float, 4096> silence;
        silence.fill(0.0f);
        
        analyzer.processBlock(silence.data(), static_cast<int>(silence.size()));
        
        auto levels = analyzer.getBarkLevels();
        for (float level : levels)
        {
            REQUIRE(level < -90.0f);
        }
    }
    
    SECTION("Sine wave gives peak in correct band")
    {
        // Gera senoide de 1000 Hz
        std::array<float, 4096> buffer;
        float phase = 0.0f;
        float phaseInc = 1000.0f / 48000.0f * 2.0f * juce::MathConstants<float>::pi;
        
        for (auto& sample : buffer)
        {
            sample = std::sin(phase) * 0.5f;
            phase += phaseInc;
        }
        
        analyzer.processBlock(buffer.data(), static_cast<int>(buffer.size()));
        
        auto levels = analyzer.getBarkLevels();
        
        // Banda 8 (1000 Hz) deve ter o maior nível
        int expectedBand = BarkAnalyzer::hzToBarkBand(1000.0f);
        float maxLevel = -100.0f;
        int maxBand = 0;
        
        for (int i = 0; i < 24; ++i)
        {
            if (levels[i] > maxLevel)
            {
                maxLevel = levels[i];
                maxBand = i;
            }
        }
        
        REQUIRE(maxBand == expectedBand);
    }
}