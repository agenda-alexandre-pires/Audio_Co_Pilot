#pragma once

#include <juce_graphics/juce_graphics.h>

namespace Colours
{
    // ============================================
    // PALETA INDUSTRIAL BRUTALIST
    // Inspirado em SSL, Avid, equipamentos profissionais
    // ============================================

    // Backgrounds
    const juce::Colour Background       { 0xFF1A1A1A };  // Slate escuro principal
    const juce::Colour BackgroundLight  { 0xFF2A2A2A };  // Painéis elevados
    const juce::Colour BackgroundDark   { 0xFF0F0F0F };  // Áreas rebaixadas

    // Textos
    const juce::Colour TextPrimary      { 0xFFE0E0E0 };  // Texto principal
    const juce::Colour TextSecondary    { 0xFF808080 };  // Texto secundário
    const juce::Colour TextDisabled     { 0xFF505050 };  // Texto desabilitado

    // Accent - Verde Smaart
    const juce::Colour Accent           { 0xFF00FF00 };  // Verde principal
    const juce::Colour AccentDim        { 0xFF00AA00 };  // Verde escuro
    const juce::Colour AccentBright     { 0xFF00FF66 };  // Verde claro

    // Status
    const juce::Colour Safe             { 0xFF00FF00 };  // Verde - OK
    const juce::Colour Warning          { 0xFFFFB000 };  // Âmbar - Atenção
    const juce::Colour Alert            { 0xFFFF0000 };  // Vermelho - Perigo
    const juce::Colour Info             { 0xFF00AAFF };  // Azul - Informação

    // Meters
    const juce::Colour MeterGreen       { 0xFF00CC00 };
    const juce::Colour MeterYellow      { 0xFFCCCC00 };
    const juce::Colour MeterOrange      { 0xFFFF8800 };
    const juce::Colour MeterRed         { 0xFFFF0000 };
    const juce::Colour MeterBackground  { 0xFF0A0A0A };
    const juce::Colour MeterOutline     { 0xFF333333 };

    // Bordas e separadores
    const juce::Colour Border           { 0xFF333333 };
    const juce::Colour BorderLight      { 0xFF444444 };
    const juce::Colour Separator        { 0xFF252525 };
}

