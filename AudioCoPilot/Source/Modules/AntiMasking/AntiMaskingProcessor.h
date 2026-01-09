#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "BarkAnalyzer.h"
#include "MaskingCalculator.h"
#include <array>
#include <memory>

namespace AudioCoPilot
{
    /**
     * @brief Processador principal do módulo Anti-Masking
     * 
     * Gerencia 4 canais de entrada (1 target + 3 maskers), realiza análise
     * Bark em cada um, e calcula mascaramento em tempo real.
     */
    class AntiMaskingProcessor
    {
    public:
        static constexpr int NUM_CHANNELS = 4;  // 1 target + 3 maskers
        static constexpr int TARGET_CHANNEL = 0;
        
        AntiMaskingProcessor();
        ~AntiMaskingProcessor() = default;
        
        /**
         * @brief Prepara o processador
         * @param sampleRate Taxa de amostragem
         * @param blockSize Tamanho do bloco de processamento
         */
        void prepare(double sampleRate, int blockSize);
        
        /**
         * @brief Processa um bloco de áudio
         * @param audioBuffer Buffer com os 4 canais
         */
        void processBlock(const juce::AudioBuffer<float>& audioBuffer);
        
        /**
         * @brief Define qual canal de entrada é o target
         * @param channelIndex Índice do canal (0-3)
         */
        void setTargetChannel(int channelIndex);
        
        /**
         * @brief Define quais canais são maskers
         * @param maskerIndex Índice do masker (0-2)
         * @param channelIndex Índice do canal de áudio
         * @param enabled Se o masker está ativo
         */
        void setMaskerChannel(int maskerIndex, int channelIndex, bool enabled);
        
        /**
         * @brief Retorna o resultado atual da análise (média de 5 segundos)
         */
        const MaskingAnalysisResult& getCurrentResult() const;
        
        /**
         * @brief Retorna o espectro Bark de um canal específico
         * @param channelIndex Índice do canal
         */
        const std::array<float, 24>& getChannelBarkSpectrum(int channelIndex) const;
        
        /**
         * @brief Reseta toda a análise
         */
        void reset();
        
        /**
         * @brief Retorna se o processador está pronto
         */
        bool isPrepared() const { return _prepared; }
        
    private:
        bool _prepared = false;
        double _sampleRate = 48000.0;
        
        // Analisadores Bark para cada canal (criados lazy para evitar travamento)
        std::array<std::unique_ptr<BarkAnalyzer>, NUM_CHANNELS> _barkAnalyzers;
        
        // Calculador de mascaramento
        MaskingCalculator _maskingCalculator;
        
        // Garante que um analisador existe (criação lazy)
        void ensureAnalyzer(int index);
        
        // Mapeamento de canais
        int _targetChannel = 0;
        std::array<int, 3> _maskerChannels = {1, 2, 3};
        std::array<bool, 3> _maskerEnabled = {false, false, false};
        
        // Cache dos espectros
        std::array<std::array<float, 24>, NUM_CHANNELS> _cachedSpectrums;
        
        // Timer para atualização da média
        int _updateCounter = 0;
        int _updateInterval = 0;  // Em blocos
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AntiMaskingProcessor)
    };
}