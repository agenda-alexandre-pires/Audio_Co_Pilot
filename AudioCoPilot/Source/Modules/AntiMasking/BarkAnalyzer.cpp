#include "BarkAnalyzer.h"
#include <cmath>

namespace AudioCoPilot
{
    // Bordas das bandas críticas em Hz (25 valores para 24 bandas)
    const std::array<float, 25> BarkAnalyzer::BARK_EDGES = {
        0.0f, 100.0f, 200.0f, 300.0f, 400.0f, 510.0f, 630.0f, 770.0f,
        920.0f, 1080.0f, 1270.0f, 1480.0f, 1720.0f, 2000.0f, 2320.0f,
        2700.0f, 3150.0f, 3700.0f, 4400.0f, 5300.0f, 6400.0f, 7700.0f,
        9500.0f, 12000.0f, 15500.0f
    };
    
    // Centros das bandas críticas em Hz
    const std::array<float, 24> BarkAnalyzer::BARK_CENTERS = {
        50.0f, 150.0f, 250.0f, 350.0f, 455.0f, 570.0f, 700.0f, 845.0f,
        1000.0f, 1175.0f, 1375.0f, 1600.0f, 1860.0f, 2160.0f, 2510.0f,
        2925.0f, 3425.0f, 4050.0f, 4850.0f, 5850.0f, 7050.0f, 8600.0f,
        10750.0f, 13750.0f
    };
    
    BarkAnalyzer::BarkAnalyzer()
    {
        _fft = std::make_unique<juce::dsp::FFT>(FFT_ORDER);
        _window = std::make_unique<juce::dsp::WindowingFunction<float>>(
            FFT_SIZE, juce::dsp::WindowingFunction<float>::hann);
        
        // Inicializa arrays
        _fftInput.fill(0.0f);
        _fftOutput.fill(0.0f);
        _inputBuffer.fill(0.0f);
        _barkLevelsDB.fill(-100.0f);
        _smoothedBarkLevels.fill(-100.0f);
        _bandStartBin.fill(0);
        _bandEndBin.fill(0);
    }
    
    void BarkAnalyzer::prepare(double sampleRate)
    {
        _sampleRate = sampleRate;
        _inputBufferIndex = 0;
        
        // Coeficiente de smoothing (~100ms)
        _smoothingCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.1f / FFT_SIZE));
        
        calculateBinMapping();
        
        // Reset níveis
        _barkLevelsDB.fill(-100.0f);
        _smoothedBarkLevels.fill(-100.0f);
    }
    
    void BarkAnalyzer::calculateBinMapping()
    {
        float binWidth = static_cast<float>(_sampleRate) / FFT_SIZE;
        
        for (int band = 0; band < NUM_BARK_BANDS; ++band)
        {
            float lowFreq = BARK_EDGES[band];
            float highFreq = BARK_EDGES[band + 1];
            
            _bandStartBin[band] = static_cast<int>(std::ceil(lowFreq / binWidth));
            _bandEndBin[band] = static_cast<int>(std::floor(highFreq / binWidth));
            
            // Garante pelo menos uma bin por banda
            if (_bandEndBin[band] < _bandStartBin[band])
                _bandEndBin[band] = _bandStartBin[band];
            
            // Limita ao Nyquist
            int maxBin = FFT_SIZE / 2 - 1;
            _bandStartBin[band] = std::min(_bandStartBin[band], maxBin);
            _bandEndBin[band] = std::min(_bandEndBin[band], maxBin);
        }
    }
    
    void BarkAnalyzer::processBlock(const float* samples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            _inputBuffer[_inputBufferIndex++] = samples[i];
            
            if (_inputBufferIndex >= FFT_SIZE)
            {
                performFFT();
                calculateBarkLevels();
                
                // 50% overlap
                std::copy(_inputBuffer.begin() + FFT_SIZE / 2,
                          _inputBuffer.end(),
                          _inputBuffer.begin());
                _inputBufferIndex = FFT_SIZE / 2;
            }
        }
    }
    
    void BarkAnalyzer::performFFT()
    {
        // Copia para buffer FFT
        std::copy(_inputBuffer.begin(), _inputBuffer.end(), _fftInput.begin());
        
        // Aplica janela
        _window->multiplyWithWindowingTable(_fftInput.data(), FFT_SIZE);
        
        // Copia para output buffer (FFT in-place precisa de espaço 2x)
        std::copy(_fftInput.begin(), _fftInput.end(), _fftOutput.begin());
        std::fill(_fftOutput.begin() + FFT_SIZE, _fftOutput.end(), 0.0f);
        
        // Executa FFT
        _fft->performRealOnlyForwardTransform(_fftOutput.data());
    }
    
    void BarkAnalyzer::calculateBarkLevels()
    {
        for (int band = 0; band < NUM_BARK_BANDS; ++band)
        {
            float sumPower = 0.0f;
            int numBins = 0;
            
            for (int bin = _bandStartBin[band]; bin <= _bandEndBin[band]; ++bin)
            {
                // FFT output: real e imag intercalados
                float real = _fftOutput[bin * 2];
                float imag = _fftOutput[bin * 2 + 1];
                float magnitude = std::sqrt(real * real + imag * imag);
                sumPower += magnitude * magnitude;
                ++numBins;
            }
            
            // Média de potência na banda
            float avgPower = (numBins > 0) ? sumPower / numBins : 0.0f;
            
            // Normalização
            avgPower /= (FFT_SIZE * FFT_SIZE);
            
            // Converte para dB
            float levelDB = (avgPower > 1e-10f) ? 10.0f * std::log10(avgPower) : -100.0f;
            _barkLevelsDB[band] = levelDB;
            
            // Smoothing
            _smoothedBarkLevels[band] = _smoothingCoeff * _smoothedBarkLevels[band] +
                                        (1.0f - _smoothingCoeff) * levelDB;
        }
    }
    
    int BarkAnalyzer::hzToBarkBand(float hz)
    {
        for (int i = 0; i < NUM_BARK_BANDS; ++i)
        {
            if (hz < BARK_EDGES[i + 1])
                return i;
        }
        return NUM_BARK_BANDS - 1;
    }
    
    float BarkAnalyzer::hzToBark(float hz)
    {
        // Fórmula de Traunmüller (1990)
        return 26.81f * hz / (1960.0f + hz) - 0.53f;
    }
    
    float BarkAnalyzer::barkToHz(float bark)
    {
        return 1960.0f * (bark + 0.53f) / (26.28f - bark);
    }
    
    float BarkAnalyzer::getBandCenterHz(int band)
    {
        if (band >= 0 && band < NUM_BARK_BANDS)
            return BARK_CENTERS[band];
        return 0.0f;
    }
    
    std::pair<float, float> BarkAnalyzer::getBandEdgesHz(int band)
    {
        if (band >= 0 && band < NUM_BARK_BANDS)
            return {BARK_EDGES[band], BARK_EDGES[band + 1]};
        return {0.0f, 0.0f};
    }
}