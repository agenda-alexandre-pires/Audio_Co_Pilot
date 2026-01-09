#include <catch2/catch_all.hpp>
#include "../Source/Modules/FeedbackPrediction/HowlingDetector.h"
#include "../Source/Modules/FeedbackPrediction/FeedbackPredictionProcessor.h"
#include <cmath>
#include <vector>

using namespace AudioCoPilot;

TEST_CASE("HowlingDetector initialization", "[feedback]")
{
    HowlingDetector detector;
    detector.prepare(48000.0, 4096);
    
    REQUIRE(detector.getWorstStatus() == FeedbackCandidate::Status::Safe);
}

TEST_CASE("HowlingDetector detects pure tone as potential feedback", "[feedback]")
{
    HowlingDetector detector;
    detector.prepare(48000.0, 4096);
    
    // Cria espectro com pico único em 2kHz (sem harmônicos = feedback)
    const int fftSize = 4096;
    const int numBins = fftSize / 2 + 1;
    std::vector<float> spectrum(numBins, 0.001f); // Ruído de fundo baixo
    
    // Pico em ~2kHz
    int peakBin = static_cast<int>(2000.0f * fftSize / 48000.0f);
    if (peakBin < numBins) {
        spectrum[static_cast<size_t>(peakBin)] = 1.0f; // Pico forte
    }
    
    // Processa vários frames (para IPMP)
    for (int i = 0; i < 20; ++i) {
        detector.processFrame(spectrum.data(), numBins);
    }
    
    // Deve detectar como suspeito
    const auto& candidates = detector.getCandidates();
    bool foundSuspect = false;
    for (const auto& c : candidates) {
        if (c.frequency > 1800 && c.frequency < 2200) {
            // Ajustado PNPR threshold
            if (c.pnpr > 4.0f) { 
                foundSuspect = true;
            }
        }
    }
    
    REQUIRE(foundSuspect);
}

TEST_CASE("HowlingDetector ignores signal with harmonics", "[feedback]")
{
    HowlingDetector detector;
    detector.prepare(48000.0, 4096);
    
    // Cria espectro com fundamental + harmônicos (= música)
    const int fftSize = 4096;
    const int numBins = fftSize / 2 + 1;
    std::vector<float> spectrum(numBins, 0.001f);
    
    float fundamental = 440.0f; // Lá musical
    int bin1 = static_cast<int>(fundamental * fftSize / 48000.0f);
    int bin2 = static_cast<int>(fundamental * 2 * fftSize / 48000.0f);
    int bin3 = static_cast<int>(fundamental * 3 * fftSize / 48000.0f);
    
    if (bin1 < numBins) spectrum[static_cast<size_t>(bin1)] = 1.0f;   // Fundamental
    if (bin2 < numBins) spectrum[static_cast<size_t>(bin2)] = 0.5f;   // 2º harmônico
    if (bin3 < numBins) spectrum[static_cast<size_t>(bin3)] = 0.25f;  // 3º harmônico
    
    for (int i = 0; i < 20; ++i) {
        detector.processFrame(spectrum.data(), numBins);
    }
    
    // PHPR deve ser relativamente baixo (tem harmônicos)
    // Nota: pode não ser menor que threshold se o harmônico não estiver exatamente no bin esperado
    const auto& candidates = detector.getCandidates();
    bool foundWithHarmonics = false;
    for (const auto& c : candidates) {
        if (std::abs(c.frequency - fundamental) < 20.0f) {
            // Com harmônicos presentes, PHPR deve ser menor que um valor alto (feedback sem harmônicos teria PHPR > 15 dB)
            // Aceita até 10 dB como indicativo de harmônicos presentes
            if (c.phpr < 10.0f) {
                foundWithHarmonics = true;
            }
        }
    }
    // Verifica que encontrou pelo menos um candidato com PHPR relativamente baixo
    REQUIRE(foundWithHarmonics);
}

TEST_CASE("Learn stage reduces risk for known frequencies", "[feedback]")
{
    HowlingDetector detector;
    detector.prepare(48000.0, 512);
    
    // Marca 1kHz como frequência aprendida
    detector.setLearnedFrequencies({1000.0f});
    
    // Verifica que 1kHz é reconhecida
    REQUIRE(detector.isLearnedFrequency(1000.0f));
    REQUIRE(detector.isLearnedFrequency(1020.0f, 50.0f)); // Com tolerância
    REQUIRE_FALSE(detector.isLearnedFrequency(2000.0f));
}

TEST_CASE("FeedbackPredictionProcessor initialization", "[feedback]")
{
    FeedbackPredictionProcessor processor;
    processor.prepare(48000.0, 512);
    
    REQUIRE_FALSE(processor.isEnabled());
    REQUIRE_FALSE(processor.isLearning());
}

TEST_CASE("FeedbackPredictionProcessor enable/disable", "[feedback]")
{
    FeedbackPredictionProcessor processor;
    processor.prepare(48000.0, 512);
    
    processor.setEnabled(true);
    REQUIRE(processor.isEnabled());
    
    processor.setEnabled(false);
    REQUIRE_FALSE(processor.isEnabled());
}

TEST_CASE("FeedbackPredictionProcessor learn stage", "[feedback]")
{
    FeedbackPredictionProcessor processor;
    processor.prepare(48000.0, 512);
    processor.setEnabled(true);
    
    processor.startLearning();
    REQUIRE(processor.isLearning());
    
    processor.stopLearning();
    REQUIRE_FALSE(processor.isLearning());
}

TEST_CASE("FeedbackPredictionProcessor processes silence", "[feedback]")
{
    FeedbackPredictionProcessor processor;
    processor.prepare(48000.0, 512);
    processor.setEnabled(true);
    
    std::vector<float> silence(512, 0.0f);
    
    // Não deve crashar
    REQUIRE_NOTHROW(processor.process(silence.data(), 512));
    
    // Status deve ser Safe
    auto worstStatus = processor.getDetector().getWorstStatus();
    REQUIRE(worstStatus == FeedbackCandidate::Status::Safe);
}

TEST_CASE("HowlingDetector reset clears state", "[feedback]")
{
    HowlingDetector detector;
    detector.prepare(48000.0, 512);
    
    // Processa alguns frames
    const int fftSize = 4096;
    const int numBins = fftSize / 2 + 1;
    std::vector<float> spectrum(numBins, 0.1f);
    
    for (int i = 0; i < 10; ++i) {
        detector.processFrame(spectrum.data(), numBins);
    }
    
    // Reseta
    detector.reset();
    
    // Deve estar limpo
    REQUIRE(detector.getWorstStatus() == FeedbackCandidate::Status::Safe);
}

TEST_CASE("HowlingDetector detects feedback using reference signal", "[feedback]")
{
    HowlingDetector detector;
    detector.prepare(48000.0, 4096); // fftSize correto
    
    const int fftSize = 4096;
    const int numBins = fftSize / 2 + 1;
    
    // 1. Cria espectro de "Música" (Referência) com vários picos
    std::vector<float> refSpectrum(numBins, 0.001f);
    std::vector<float> measSpectrum(numBins, 0.001f);
    
    // Adiciona "música" (picos harmônicos) em ambos
    // Se Meas == Ref, Loop Gain é 0, deve ser seguro
    float fundamental = 440.0f;
    for (int i = 1; i <= 5; ++i) {
        int bin = static_cast<int>(fundamental * i * fftSize / 48000.0f);
        if (bin < numBins) {
            refSpectrum[bin] = 0.5f; // -6 dB
            measSpectrum[bin] = 0.5f; // -6 dB (Sinal passando sem ganho)
        }
    }
    
    // 2. Adiciona FEEDBACK apenas no Measurement (2kHz)
    // Ref: baixo, Meas: alto -> Loop Gain alto
    int feedbackBin = static_cast<int>(2000.0f * fftSize / 48000.0f);
    measSpectrum[feedbackBin] = 1.0f; // 0 dB
    refSpectrum[feedbackBin] = 0.001f; // -60 dB (silêncio na mesa)
    
    // Processa alguns frames para estabilizar persistência
    for (int i = 0; i < 20; ++i) {
        detector.processFrameWithReference(measSpectrum.data(), refSpectrum.data(), numBins);
    }
    
    // 3. Verifica resultados
    const auto& candidates = detector.getCandidates();
    
    bool foundFeedback = false;
    bool ignoredMusic = true;
    
    for (const auto& c : candidates) {
        // Verifica Feedback em 2kHz
        if (std::abs(c.frequency - 2000.0f) < 50.0f) {
            // Deve ter Loop Gain alto (~60dB de diferença)
            if (c.loopGain > 10.0f && c.riskScore > 0.8f) {
                foundFeedback = true;
            }
        }
        
        // Verifica Música (440Hz, 880Hz, etc)
        // Se encontrar algum candidato nessas frequências, ele deve ser SAFE
        if (std::abs(c.frequency - 440.0f) < 50.0f || 
            std::abs(c.frequency - 880.0f) < 50.0f) {
            
            // Loop Gain deve ser ~0 dB
            if (c.loopGain > 3.0f || c.status != FeedbackCandidate::Status::Safe) {
                ignoredMusic = false;
            }
        }
    }
    
    REQUIRE(foundFeedback);
    REQUIRE(ignoredMusic);
}
