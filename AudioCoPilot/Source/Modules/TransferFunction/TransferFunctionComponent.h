#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "TransferFunctionProcessor.h"

/**
 * TransferFunctionComponent
 * 
 * Componente UI para visualizar a função de transferência:
 * - Gráfico de magnitude (dB vs frequência)
 * - Gráfico de fase (graus vs frequência)
 * - Escala logarítmica de frequência
 */
class TransferFunctionComponent : public juce::Component,
                                  public juce::Timer,
                                  public juce::ComboBox::Listener
{
public:
    TransferFunctionComponent();
    ~TransferFunctionComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    /**
     * Conecta ao processador (thread-safe)
     */
    void setProcessor(TransferFunctionProcessor* processor);

private:
    TransferFunctionProcessor* _processor = nullptr;
    
    // Dados atuais para renderização
    TransferFunctionProcessor::TransferData _currentData;
    
    // Controle de resolução
    juce::ComboBox _resolutionComboBox;
    juce::Label _resolutionLabel;
    
    // Cores e estilo
    juce::Colour _magnitudeColour;
    juce::Colour _phaseColour;
    juce::Colour _gridColour;
    juce::Colour _backgroundColour;
    
    // Layout
    static constexpr float MAGNITUDE_HEIGHT_RATIO = 0.6f;  // 60% para magnitude
    static constexpr float PHASE_HEIGHT_RATIO = 0.4f;      // 40% para fase
    static constexpr float FREQ_RANGE_MIN = 20.0f;          // 20 Hz
    static constexpr float FREQ_RANGE_MAX = 20000.0f;      // 20 kHz
    static constexpr float MAGNITUDE_RANGE_MIN = -60.0f;   // -60 dB
    static constexpr float MAGNITUDE_RANGE_MAX = 20.0f;    // +20 dB
    static constexpr float PHASE_RANGE_MIN = -180.0f;      // -180 graus
    static constexpr float PHASE_RANGE_MAX = 180.0f;       // +180 graus
    
    void drawMagnitudePlot(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawPhasePlot(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds, bool isMagnitude);
    void drawTooltip(juce::Graphics& g);
    
    /**
     * Converte frequência para posição X (escala logarítmica)
     */
    float freqToX(float freq, float width) const;
    
    /**
     * Converte magnitude dB para posição Y
     */
    float magnitudeToY(float db, float height) const;
    
    /**
     * Converte fase em graus para posição Y
     */
    float phaseToY(float degrees, float height) const;
    
    /**
     * Encontra o índice do bin mais próximo de uma frequência
     */
    int findBinForFrequency(float freq) const;
    
    /**
     * Converte posição X para frequência (escala logarítmica inversa)
     */
    float xToFreq(float x, float width) const;
    
    /**
     * Informações do tooltip
     */
    struct TooltipInfo
    {
        bool visible = false;
        float frequency = 0.0f;
        float magnitudeDb = 0.0f;
        float phaseDegrees = 0.0f;
        juce::Point<float> position;
    };
    TooltipInfo _tooltipInfo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransferFunctionComponent)
};

