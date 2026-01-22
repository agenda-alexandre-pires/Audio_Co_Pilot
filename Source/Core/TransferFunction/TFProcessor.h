#pragma once

#include "../../JuceHeader.h"
#include "FFTAnalyzer.h"
#include <complex>
#include <vector>
#include <atomic>

/**
 * TFProcessor - Smaart-style Transfer Function
 * 
 * Implements H1 estimator with:
 * - Cross-spectrum averaging (Gxx, Gyy, Gxy)
 * - Exponential averaging (IIR)
 * - Delay compensation
 * - Phase unwrap
 * - Fractional-octave smoothing
 * - Coherence (gamma2)
 */
class TFProcessor
{
public:
    TFProcessor();
    ~TFProcessor();
    
    // Setup processor
    void prepare(int fftSize, double sampleRate);
    
    // Process audio buffers (called from audio thread)
    // OLD: Separate calls (removed - causes sync issues)
    // void processReference(const float* input, int numSamples);
    // void processMeasurement(const float* input, int numSamples);
    
    // NEW: Synchronized processing - both channels together
    void processBlock(const float* ref, const float* meas, int numSamples);
    
    // Get transfer function results (called from UI thread)
    void getMagnitudeResponse(std::vector<float>& magnitudeDb);
    void getPhaseResponse(std::vector<float>& phaseDegrees);
    void getCoherence(std::vector<float>& coherence);
    
    // Get frequency bins (for axis)
    void getFrequencyBins(std::vector<float>& frequencies);
    
    // Get estimated delay between channels (in seconds)
    double getEstimatedDelay() const { return estimatedDelay; }
    
    // Reset processing
    void reset();
    
    // Check if processor is ready
    bool isReady() const { return ready.load(); }
    
    // Settings
    void setAveragingTime(double seconds) { averagingTime.store(seconds); }
    double getAveragingTime() const { return averagingTime.load(); }
    
    void setSmoothingOctaves(double octaves) { smoothingOctaves.store(octaves); }
    double getSmoothingOctaves() const { return smoothingOctaves.load(); }
    
private:
    void tryProcessSynchronizedFrames();  // Process frames only when both buffers are ready
    void processFrame();
    void computeCrossSpectrum();
    void updateAverages();
    void estimateDelay();  // GCC-PHAT (fast, uses instantaneous spectrum)
    void estimateDelayPhaseBased();  // Fallback method
    void applyDelayCompensation();
    void applySmoothing();
    void unwrapPhase();
    void extractMagnitudeAndPhase();
    
    // FFT analyzers
    std::unique_ptr<FFTAnalyzer> referenceFFT;
    std::unique_ptr<FFTAnalyzer> measurementFFT;
    
    // Buffers for overlap processing
    std::vector<float> referenceBuffer;
    std::vector<float> measurementBuffer;
    
    // FFT results (complex spectra)
    std::vector<std::complex<double>> X;  // Reference spectrum
    std::vector<std::complex<double>> Y;  // Measurement spectrum
    
    // Cross-spectra (averaged)
    std::vector<double> Gxx;  // Auto-spectrum of reference
    std::vector<double> Gyy;  // Auto-spectrum of measurement
    std::vector<std::complex<double>> Gxy;  // Cross-spectrum
    
    // Transfer function
    std::vector<std::complex<double>> H;  // H1 = Gxy / Gxx (averaged)
    std::vector<std::complex<double>> H_compensated;  // With delay compensation (averaged)
    std::vector<std::complex<double>> H_smoothed;  // After smoothing (averaged)
    
    // Coherence
    std::vector<double> gamma2;  // Magnitude-squared coherence
    
    // Results for UI (double-buffered for stability)
    std::vector<float> magnitudeDb;
    std::vector<float> phaseDegrees;
    std::vector<float> coherence;
    std::vector<float> frequencies;
    
    // Double-buffered results for smooth UI updates
    std::vector<float> magnitudeDbBuffer;
    std::vector<float> phaseDegreesBuffer;
    std::vector<float> coherenceBuffer;
    juce::CriticalSection bufferLock;
    
    // Averaging state (exponential averaging with time constant)
    double averagingAlpha{0.0};  // Computed from averagingTime: alpha = exp(-frameDt / Tavg)
    std::atomic<double> averagingTime{1.5};  // seconds - time constant (1.5s for stable Smaart-like response)
    double frameDt{0.0};  // Hop time in seconds
    int frameCount{0};  // Track frame count for adaptive averaging
    static constexpr int fastAveragingFrames = 30;  // Use fast averaging for first 30 frames (~0.5s @ 60fps)
    
    // Delay compensation
    double estimatedDelay{0.0};  // in seconds
    double smoothedDelay{0.0};  // smoothed version
    int delayUpdateCounter{0};
    static constexpr int delayUpdatePeriod = 100;  // frames
    static constexpr double delayStabilityThreshold = 0.0001;  // 0.1ms threshold (was 0.05ms)
    static constexpr int delayStabilityCount = 3;  // 3 stable updates (was 5)
    
    // GCC-PHAT delay finder (fast, uses instantaneous spectrum)
    std::unique_ptr<juce::dsp::FFT> phatFFT;
    int phatFftOrder{0};
    std::vector<std::complex<float>> phatFftBuffer;  // complex spectrum, size fftSize
    std::vector<float> phatTime;                      // size fftSize
    double lastDelaySec{0.0};
    int stableDelayCount{0};
    bool delayLocked{false};
    
    // Smoothing - 1/12 octave default (Smaart-like)
    std::atomic<double> smoothingOctaves{1.0/12.0};  // 1/12 octave default (Smaart-like)
    
    // Processing parameters
    int fftSize{16384};
    double sampleRate{48000.0};
    int hopSize{0};  // NFFT * (1 - overlap)
    static constexpr double overlap = 0.75;  // 75% overlap
    
    // State
    std::atomic<bool> ready{false};
    std::atomic<bool> newDataAvailable{false};
    
    // Thread safety
    juce::CriticalSection processLock;
    
    // Constants
    static constexpr double eps = 1e-12;
    static constexpr double cohMinDraw = 0.6;
    static constexpr double cohMinMath = 0.4;
};
