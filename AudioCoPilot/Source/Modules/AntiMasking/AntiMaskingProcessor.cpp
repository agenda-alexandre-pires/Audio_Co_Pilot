#include "AntiMaskingProcessor.h"

namespace AudioCoPilot
{
    AntiMaskingProcessor::AntiMaskingProcessor()
    {
        for (auto& spectrum : _cachedSpectrums)
            spectrum.fill(-100.0f);
        
        // Inicializa os ponteiros como nullptr (criação lazy)
        for (auto& analyzer : _barkAnalyzers)
            analyzer = nullptr;
    }
    
    void AntiMaskingProcessor::ensureAnalyzer(int index)
    {
        if (index >= 0 && index < NUM_CHANNELS && !_barkAnalyzers[index])
        {
            _barkAnalyzers[index] = std::make_unique<BarkAnalyzer>();
            if (_prepared && _sampleRate > 0)
            {
                _barkAnalyzers[index]->prepare(_sampleRate);
            }
        }
    }
    
    void AntiMaskingProcessor::prepare(double sampleRate, int blockSize)
    {
        _sampleRate = sampleRate;
        
        // Prepara analisadores existentes e cria os que faltam
        for (int i = 0; i < NUM_CHANNELS; ++i)
        {
            ensureAnalyzer(i);
            if (_barkAnalyzers[i])
                _barkAnalyzers[i]->prepare(sampleRate);
        }
        
        _maskingCalculator.prepare(sampleRate);
        
        // Atualiza a média a cada ~100ms
        _updateInterval = static_cast<int>((sampleRate * 0.1) / blockSize);
        if (_updateInterval < 1) _updateInterval = 1;
        
        _updateCounter = 0;
        _prepared = true;
    }
    
    void AntiMaskingProcessor::processBlock(const juce::AudioBuffer<float>& audioBuffer)
    {
        if (!_prepared)
            return;
        
        int numSamples = audioBuffer.getNumSamples();
        int numChannels = std::min(audioBuffer.getNumChannels(), NUM_CHANNELS);
        
        // Processa cada canal pelo analisador Bark
        for (int ch = 0; ch < numChannels; ++ch)
        {
            ensureAnalyzer(ch);
            if (_barkAnalyzers[ch])
            {
                const float* channelData = audioBuffer.getReadPointer(ch);
                _barkAnalyzers[ch]->processBlock(channelData, numSamples);
                _cachedSpectrums[ch] = _barkAnalyzers[ch]->getSmoothedBarkLevels();
            }
        }
        
        // Atualiza o calculador de mascaramento
        if (_targetChannel < numChannels)
        {
            _maskingCalculator.setTargetSpectrum(_cachedSpectrums[_targetChannel]);
        }
        
        for (int m = 0; m < 3; ++m)
        {
            if (_maskerEnabled[m] && _maskerChannels[m] < numChannels)
            {
                _maskingCalculator.setMaskerSpectrum(m, _cachedSpectrums[_maskerChannels[m]]);
                _maskingCalculator.setMaskerEnabled(m, true);
            }
            else
            {
                _maskingCalculator.setMaskerEnabled(m, false);
            }
        }
        
        // Atualiza a média periodicamente
        ++_updateCounter;
        if (_updateCounter >= _updateInterval)
        {
            auto result = _maskingCalculator.calculate();
            _maskingCalculator.updateRunningAverage(result);
            _updateCounter = 0;
        }
    }
    
    void AntiMaskingProcessor::setTargetChannel(int channelIndex)
    {
        if (channelIndex >= 0 && channelIndex < NUM_CHANNELS)
            _targetChannel = channelIndex;
    }
    
    void AntiMaskingProcessor::setMaskerChannel(int maskerIndex, int channelIndex, bool enabled)
    {
        if (maskerIndex >= 0 && maskerIndex < 3)
        {
            _maskerChannels[maskerIndex] = channelIndex;
            _maskerEnabled[maskerIndex] = enabled;
        }
    }
    
    const MaskingAnalysisResult& AntiMaskingProcessor::getCurrentResult() const
    {
        return _maskingCalculator.getAveragedResult();
    }
    
    const std::array<float, 24>& AntiMaskingProcessor::getChannelBarkSpectrum(int channelIndex) const
    {
        static const std::array<float, 24> empty = {-100.0f};
        
        if (channelIndex >= 0 && channelIndex < NUM_CHANNELS)
            return _cachedSpectrums[channelIndex];
        
        return empty;
    }
    
    void AntiMaskingProcessor::reset()
    {
        _maskingCalculator.resetAverage();
        _updateCounter = 0;
        
        for (auto& spectrum : _cachedSpectrums)
            spectrum.fill(-100.0f);
    }
}