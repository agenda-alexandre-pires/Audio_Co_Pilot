#include "TransferFunctionProcessor.h"
#include <cmath>
#include <algorithm>
#include <vector>

TransferFunctionProcessor::TransferFunctionProcessor()
    : _window(FFT_SIZE, juce::dsp::WindowingFunction<float>::hann)
{
    // Inicializa buffers
    _referenceBuffer.fill(0.0f);
    _measurementBuffer.fill(0.0f);
    
    // Inicializa buffers temporários
    _refWindowedBuffer.fill(0.0f);
    _measWindowedBuffer.fill(0.0f);
    for (auto& val : _refComplexBuffer)
        val = std::complex<float>(0.0f, 0.0f);
    for (auto& val : _measComplexBuffer)
        val = std::complex<float>(0.0f, 0.0f);
    for (auto& val : _refOutputBuffer)
        val = std::complex<float>(0.0f, 0.0f);
    for (auto& val : _measOutputBuffer)
        val = std::complex<float>(0.0f, 0.0f);
    
    // Inicializa espectros
    for (auto& spec : _referenceSpectrum)
        spec = std::complex<float>(0.0f, 0.0f);
    for (auto& spec : _measurementSpectrum)
        spec = std::complex<float>(0.0f, 0.0f);
    for (auto& spec : _transferFunctionAccum)
        spec = std::complex<float>(0.0f, 0.0f);
    for (auto& spec : _transferFunction)
        spec = std::complex<float>(0.0f, 0.0f);
    
    // Inicializa dados de saída
    for (int i = 0; i < MAX_BANDS; ++i)
    {
        _magnitudeDb[i] = -100.0f;
        _phaseDegrees[i] = 0.0f;
    }
}

void TransferFunctionProcessor::prepare(double sampleRate, int maxBlockSize)
{
    _sampleRate = sampleRate;
    _bufferWritePos = 0;
    _bufferReady = false;
    _framesProcessed = 0;
    _dataReady = false;
    
    // Reset buffers
    _referenceBuffer.fill(0.0f);
    _measurementBuffer.fill(0.0f);
    
    // Reset buffers temporários
    _refWindowedBuffer.fill(0.0f);
    _measWindowedBuffer.fill(0.0f);
    for (auto& val : _refComplexBuffer)
        val = std::complex<float>(0.0f, 0.0f);
    for (auto& val : _measComplexBuffer)
        val = std::complex<float>(0.0f, 0.0f);
    for (auto& val : _refOutputBuffer)
        val = std::complex<float>(0.0f, 0.0f);
    for (auto& val : _measOutputBuffer)
        val = std::complex<float>(0.0f, 0.0f);
    
    // Reset espectros
    for (auto& spec : _referenceSpectrum)
        spec = std::complex<float>(0.0f, 0.0f);
    for (auto& spec : _measurementSpectrum)
        spec = std::complex<float>(0.0f, 0.0f);
    for (auto& spec : _transferFunctionAccum)
        spec = std::complex<float>(0.0f, 0.0f);
    for (auto& spec : _transferFunction)
        spec = std::complex<float>(0.0f, 0.0f);
    
    // Reset dados de saída
    for (int i = 0; i < MAX_BANDS; ++i)
    {
        _magnitudeDb[i] = -100.0f;
        _phaseDegrees[i] = 0.0f;
    }
}

void TransferFunctionProcessor::processBlock(const float* const* inputData, 
                                              int numChannels, 
                                              int numSamples)
{
    if (!_enabled.load() || numChannels < 2)
        return;
    
    const float* reference = inputData[0];
    const float* measurement = inputData[1];
    
    if (reference == nullptr || measurement == nullptr)
        return;
    
    // Calcula RMS dos canais para debug
    float refSumSq = 0.0f;
    float measSumSq = 0.0f;
    
    // Copia dados para buffers circulares
    for (int i = 0; i < numSamples; ++i)
    {
        _referenceBuffer[_bufferWritePos] = reference[i];
        _measurementBuffer[_bufferWritePos] = measurement[i];
        
        refSumSq += reference[i] * reference[i];
        measSumSq += measurement[i] * measurement[i];
        
        _bufferWritePos = (_bufferWritePos + 1) % FFT_SIZE;
        
        // Quando o buffer está cheio, marca como pronto
        if (_bufferWritePos == 0)
            _bufferReady = true;
    }
    
    // Atualiza níveis RMS (média móvel simples)
    float refRms = std::sqrt(refSumSq / numSamples);
    float measRms = std::sqrt(measSumSq / numSamples);
    _referenceLevel.store(refRms, std::memory_order_relaxed);
    _measurementLevel.store(measRms, std::memory_order_relaxed);
    
    // Se o buffer está pronto, processa FFT
    if (_bufferReady)
    {
        performFFT();
        
        _framesProcessed++;
        
        // Atualiza dados de saída periodicamente (média de vários frames)
        if (_framesProcessed >= FRAMES_TO_AVERAGE)
        {
            // Calcula média da função de transferência acumulada
            calculateTransferFunction();
            updateOutputData();
            
            // Reset acumulador para próxima média
            for (auto& spec : _transferFunctionAccum)
                spec = std::complex<float>(0.0f, 0.0f);
            _framesProcessed = 0;
        }
    }
}

void TransferFunctionProcessor::performFFT()
{
    // Copia do buffer circular - sempre lê os últimos FFT_SIZE samples
    // Quando _bufferWritePos == 0, acabamos de escrever na posição 0
    // Os últimos FFT_SIZE samples estão nas posições [1, 2, ..., FFT_SIZE-1, 0]
    // (mais antigo primeiro, mais recente por último)
    // Mas para análise de fase, queremos ordem temporal correta: [0, 1, 2, ..., FFT_SIZE-1]
    // onde 0 é o mais antigo e FFT_SIZE-1 é o mais recente
    // Então lemos começando de _bufferWritePos (que é 0 quando buffer está cheio)
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        int idx = (_bufferWritePos + i) % FFT_SIZE;
        _refWindowedBuffer[i] = _referenceBuffer[idx];
        _measWindowedBuffer[i] = _measurementBuffer[idx];
    }
    
    _window.multiplyWithWindowingTable(_refWindowedBuffer.data(), FFT_SIZE);
    _window.multiplyWithWindowingTable(_measWindowedBuffer.data(), FFT_SIZE);
    
    // Converte para complexo (parte imaginária = 0)
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        _refComplexBuffer[i] = std::complex<float>(_refWindowedBuffer[i], 0.0f);
        _measComplexBuffer[i] = std::complex<float>(_measWindowedBuffer[i], 0.0f);
    }
    
    // Executa FFT (forward, não inverse)
    _fft.perform(_refComplexBuffer.data(), _refOutputBuffer.data(), false);
    _fft.perform(_measComplexBuffer.data(), _measOutputBuffer.data(), false);
    
    // Copia espectros (apenas bins positivos até Nyquist)
    for (int i = 0; i < NUM_BINS; ++i)
    {
        _referenceSpectrum[i] = _refOutputBuffer[i];
        _measurementSpectrum[i] = _measOutputBuffer[i];
    }
    
    // Calcula função de transferência para este frame
    // H(f) = Y(f) / X(f) - calculamos frame a frame e depois fazemos média
    for (int i = 0; i < NUM_BINS; ++i)
    {
        const auto& X = _referenceSpectrum[i];
        const auto& Y = _measurementSpectrum[i];
        
        float Xmag = std::abs(X);
        if (Xmag < 1e-10f)
        {
            // Se não há sinal de referência, não acumula
            continue;
        }
        
        // Calcula H(f) para este frame
        std::complex<float> H_frame = Y / X;
        
        // Acumula para média (peso igual para todos os frames)
        _transferFunctionAccum[i] += H_frame;
    }
}

void TransferFunctionProcessor::calculateTransferFunction()
{
    // Calcula média da função de transferência acumulada
    // H_avg(f) = média de H_frame(f) para todos os frames processados
    float invFrames = 1.0f / static_cast<float>(_framesProcessed);
    
    for (int i = 0; i < NUM_BINS; ++i)
    {
        // Média da função de transferência acumulada
        _transferFunction[i] = _transferFunctionAccum[i] * invFrames;
    }
}

void TransferFunctionProcessor::updateOutputData()
{
    // Calcula bandas de oitava a partir da função de transferência
    std::vector<float> bandMagnitudes;
    std::vector<float> bandPhases;
    std::vector<float> bandFrequencies;
    
    calculateOctaveBands(_transferFunction, bandMagnitudes, bandPhases, bandFrequencies);
    
    // Limpa dados antigos
    for (int i = 0; i < MAX_BANDS; ++i)
    {
        _magnitudeDb[i].store(-100.0f, std::memory_order_release);
        _phaseDegrees[i].store(0.0f, std::memory_order_release);
    }
    
    // Armazena novos dados (limita ao tamanho do array)
    size_t numBands = std::min(bandMagnitudes.size(), static_cast<size_t>(MAX_BANDS));
    for (size_t i = 0; i < numBands; ++i)
    {
        _magnitudeDb[i].store(bandMagnitudes[i], std::memory_order_release);
        _phaseDegrees[i].store(bandPhases[i], std::memory_order_release);
    }
    
    // Limpa bandas restantes se houver menos bandas que antes
    for (size_t i = numBands; i < static_cast<size_t>(MAX_BANDS); ++i)
    {
        _magnitudeDb[i].store(-100.0f, std::memory_order_release);
        _phaseDegrees[i].store(0.0f, std::memory_order_release);
    }
    
    _dataReady.store(true, std::memory_order_release);
}

TransferFunctionProcessor::TransferData TransferFunctionProcessor::readTransferData()
{
    TransferData data;
    
    if (!_dataReady.load(std::memory_order_acquire))
    {
        data.numBins = 0;
        return data;
    }
    
    double sr = _sampleRate.load();
    OctaveResolution resolution = _octaveResolution.load();
    
    // Calcula frequências das bandas de oitava
    std::vector<float> bandFrequencies = calculateOctaveBandFrequencies(sr, resolution);
    
    data.sampleRate = sr;
    data.numBins = static_cast<int>(bandFrequencies.size());
    data.magnitudeDb.resize(bandFrequencies.size());
    data.phaseDegrees.resize(bandFrequencies.size());
    data.frequencies = bandFrequencies;
    
    // Lê dados atomicamente (até o número de bandas, respeitando limite do array)
    size_t numBandsToRead = std::min(bandFrequencies.size(), static_cast<size_t>(MAX_BANDS));
    for (size_t i = 0; i < numBandsToRead; ++i)
    {
        data.magnitudeDb[i] = _magnitudeDb[i].load(std::memory_order_acquire);
        data.phaseDegrees[i] = _phaseDegrees[i].load(std::memory_order_acquire);
    }
    
    // Ajusta numBins se foi limitado
    if (bandFrequencies.size() > static_cast<size_t>(MAX_BANDS))
    {
        data.numBins = MAX_BANDS;
        data.magnitudeDb.resize(MAX_BANDS);
        data.phaseDegrees.resize(MAX_BANDS);
        data.frequencies.resize(MAX_BANDS);
    }
    
    return data;
}

void TransferFunctionProcessor::reset()
{
    _bufferWritePos = 0;
    _bufferReady = false;
    _framesProcessed = 0;
    _dataReady = false;
    
    _referenceBuffer.fill(0.0f);
    _measurementBuffer.fill(0.0f);
    
    // Reset buffers temporários
    _refWindowedBuffer.fill(0.0f);
    _measWindowedBuffer.fill(0.0f);
    for (auto& val : _refComplexBuffer)
        val = std::complex<float>(0.0f, 0.0f);
    for (auto& val : _measComplexBuffer)
        val = std::complex<float>(0.0f, 0.0f);
    for (auto& val : _refOutputBuffer)
        val = std::complex<float>(0.0f, 0.0f);
    for (auto& val : _measOutputBuffer)
        val = std::complex<float>(0.0f, 0.0f);
    
    for (auto& spec : _referenceSpectrum)
        spec = std::complex<float>(0.0f, 0.0f);
    for (auto& spec : _measurementSpectrum)
        spec = std::complex<float>(0.0f, 0.0f);
    for (auto& spec : _transferFunctionAccum)
        spec = std::complex<float>(0.0f, 0.0f);
    for (auto& spec : _transferFunction)
        spec = std::complex<float>(0.0f, 0.0f);
    
    for (int i = 0; i < MAX_BANDS; ++i)
    {
        _magnitudeDb[i] = -100.0f;
        _phaseDegrees[i] = 0.0f;
    }
    
    _referenceLevel = 0.0f;
    _measurementLevel = 0.0f;
}

TransferFunctionProcessor::DebugInfo TransferFunctionProcessor::getDebugInfo() const
{
    DebugInfo info;
    info.bufferReady = _bufferReady;
    info.framesProcessed = _framesProcessed;
    info.dataReady = _dataReady.load();
    info.referenceLevel = _referenceLevel.load();
    info.measurementLevel = _measurementLevel.load();
    return info;
}

void TransferFunctionProcessor::setOctaveResolution(OctaveResolution resolution)
{
    _octaveResolution = resolution;
    // Força recálculo na próxima atualização
    _dataReady = false;
}

std::vector<float> TransferFunctionProcessor::calculateOctaveBandFrequencies(double sampleRate, OctaveResolution resolution)
{
    std::vector<float> frequencies;
    
    // Frequências padrão ISO baseadas em 1000 Hz
    // Bandas de oitava: f_center = 1000 * 2^(n/N) onde N é o número de bandas por oitava
    int bandsPerOctave = static_cast<int>(resolution);
    
    // Calcula frequências de 20 Hz a 20 kHz
    float fMin = 20.0f;
    float fMax = 20000.0f;
    
    // Encontra a primeira banda abaixo de fMin
    float fRef = 1000.0f;  // Frequência de referência
    int nStart = static_cast<int>(std::floor(bandsPerOctave * std::log2(fMin / fRef)));
    int nEnd = static_cast<int>(std::ceil(bandsPerOctave * std::log2(fMax / fRef)));
    
    for (int n = nStart; n <= nEnd; ++n)
    {
        float fCenter = fRef * std::pow(2.0f, static_cast<float>(n) / static_cast<float>(bandsPerOctave));
        
        if (fCenter >= fMin && fCenter <= fMax)
        {
            frequencies.push_back(fCenter);
        }
    }
    
    return frequencies;
}

void TransferFunctionProcessor::calculateOctaveBands(const std::array<std::complex<float>, NUM_BINS>& spectrum,
                                                      std::vector<float>& bandMagnitudes,
                                                      std::vector<float>& bandPhases,
                                                      std::vector<float>& bandFrequencies)
{
    double sr = _sampleRate.load();
    OctaveResolution resolution = _octaveResolution.load();
    
    // Calcula frequências das bandas
    bandFrequencies = calculateOctaveBandFrequencies(sr, resolution);
    bandMagnitudes.clear();
    bandPhases.clear();
    bandMagnitudes.reserve(bandFrequencies.size());
    bandPhases.reserve(bandFrequencies.size());
    
    int bandsPerOctave = static_cast<int>(resolution);
    
    for (float fCenter : bandFrequencies)
    {
        // Calcula limites da banda (banda de oitava tem largura proporcional)
        float fLower = fCenter / std::pow(2.0f, 1.0f / (2.0f * static_cast<float>(bandsPerOctave)));
        float fUpper = fCenter * std::pow(2.0f, 1.0f / (2.0f * static_cast<float>(bandsPerOctave)));
        
        // Encontra bins da FFT correspondentes
        int binLower = static_cast<int>(std::floor(fLower * FFT_SIZE / sr));
        int binUpper = static_cast<int>(std::ceil(fUpper * FFT_SIZE / sr));
        
        binLower = juce::jlimit(0, NUM_BINS - 1, binLower);
        binUpper = juce::jlimit(0, NUM_BINS - 1, binUpper);
        
        // Média ponderada dos bins na banda (magnitude e fase separadamente)
        std::complex<float> sum = std::complex<float>(0.0f, 0.0f);
        float weightSum = 0.0f;
        
        for (int bin = binLower; bin <= binUpper; ++bin)
        {
            const auto& H = spectrum[bin];
            float magnitude = std::abs(H);
            
            // Peso baseado na magnitude (bins com mais energia têm mais peso)
            float weight = magnitude * magnitude;  // Peso quadrático
            
            sum += H * weight;
            weightSum += weight;
        }
        
        // Calcula magnitude e fase médias
        if (weightSum > 1e-10f)
        {
            std::complex<float> avg = sum / weightSum;
            float magnitude = std::abs(avg);
            float magnitudeDb = (magnitude > 1e-10f) 
                ? 20.0f * std::log10(magnitude) 
                : -100.0f;
            
            float phaseRadians = std::arg(avg);
            float phaseDegrees = phaseRadians * 180.0f / juce::MathConstants<float>::pi;
            
            // Normaliza fase para [-180, 180]
            while (phaseDegrees > 180.0f)
                phaseDegrees -= 360.0f;
            while (phaseDegrees < -180.0f)
                phaseDegrees += 360.0f;
            
            bandMagnitudes.push_back(magnitudeDb);
            bandPhases.push_back(phaseDegrees);
        }
        else
        {
            bandMagnitudes.push_back(-100.0f);
            bandPhases.push_back(0.0f);
        }
    }
}

