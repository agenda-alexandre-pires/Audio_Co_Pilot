#include "TrafficLightComponent.h"
#include <cmath>

namespace AudioCoPilot {

TrafficLightComponent::TrafficLightComponent() {
    startTimerHz(30); // 30 FPS para animação
}

TrafficLightComponent::~TrafficLightComponent() {
    stopTimer();
}

void TrafficLightComponent::timerCallback() {
    _pulsePhase += 0.1f;
    if (_pulsePhase > juce::MathConstants<float>::twoPi) {
        _pulsePhase -= juce::MathConstants<float>::twoPi;
    }
    repaint();
}

void TrafficLightComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRoundedRectangle(bounds, 8.0f);
    
    // Área do semáforo
    auto lightArea = bounds.reduced(10.0f);
    float lightDiameter = juce::jmin(lightArea.getWidth(), lightArea.getHeight() * 0.6f);
    
    // Posição central para a luz
    float lightX = lightArea.getCentreX() - lightDiameter / 2;
    float lightY = lightArea.getY() + 10.0f;
    
    // Cor da luz com pulsação para Warning/Danger
    auto colour = getStatusColour();
    
    if (_status == FeedbackCandidate::Status::Warning || 
        _status == FeedbackCandidate::Status::Danger ||
        _status == FeedbackCandidate::Status::Feedback) {
        float pulse = 0.7f + 0.3f * std::sin(_pulsePhase * 3.0f);
        colour = colour.withAlpha(pulse);
    }
    
    // Glow effect
    for (int i = 3; i > 0; --i) {
        float expand = static_cast<float>(i) * 4.0f;
        g.setColour(colour.withAlpha(0.1f * static_cast<float>(4 - i)));
        g.fillEllipse(lightX - expand, lightY - expand, 
                      lightDiameter + expand * 2, lightDiameter + expand * 2);
    }
    
    // Luz principal
    g.setColour(colour);
    g.fillEllipse(lightX, lightY, lightDiameter, lightDiameter);
    
    // Borda da luz
    g.setColour(juce::Colours::black);
    g.drawEllipse(lightX, lightY, lightDiameter, lightDiameter, 2.0f);
    
    // Texto de status
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 16.0f, juce::Font::bold)));
    
    juce::String statusText = getStatusText();
    g.drawText(statusText, 
               bounds.getX(), lightY + lightDiameter + 10.0f,
               bounds.getWidth(), 24.0f,
               juce::Justification::centred);
    
    // Frequência (se houver)
    if (_frequency > 0.0f && _status != FeedbackCandidate::Status::Safe) {
        g.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));
        juce::String freqText;
        if (_frequency >= 1000.0f) {
            freqText = juce::String(_frequency / 1000.0f, 1) + " kHz";
        } else {
            freqText = juce::String(static_cast<int>(_frequency)) + " Hz";
        }
        g.drawText(freqText,
                   bounds.getX(), lightY + lightDiameter + 34.0f,
                   bounds.getWidth(), 20.0f,
                   juce::Justification::centred);
    }
    
    // Risk score bar
    float barY = bounds.getBottom() - 30.0f;
    float barWidth = bounds.getWidth() - 20.0f;
    float barHeight = 8.0f;
    
    // Background da barra
    g.setColour(juce::Colour(0xff333333));
    g.fillRoundedRectangle(10.0f, barY, barWidth, barHeight, 4.0f);
    
    // Fill da barra
    g.setColour(getStatusColour());
    g.fillRoundedRectangle(10.0f, barY, barWidth * _riskScore, barHeight, 4.0f);
    
    // Label
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    g.drawText("RISK", 10.0f, barY - 14.0f, barWidth, 12.0f, juce::Justification::left);
    g.drawText(juce::String(static_cast<int>(_riskScore * 100)) + "%", 
               10.0f, barY - 14.0f, barWidth, 12.0f, juce::Justification::right);
}

void TrafficLightComponent::resized() {
    // Nada especial
}

juce::Colour TrafficLightComponent::getStatusColour() const {
    switch (_status) {
        case FeedbackCandidate::Status::Safe:
            return juce::Colour(0xff00cc66); // Verde
        case FeedbackCandidate::Status::Warning:
            return juce::Colour(0xffffcc00); // Amarelo
        case FeedbackCandidate::Status::Danger:
            return juce::Colour(0xffff6600); // Laranja
        case FeedbackCandidate::Status::Feedback:
            return juce::Colour(0xffff0000); // Vermelho
        default:
            return juce::Colours::grey;
    }
}

juce::String TrafficLightComponent::getStatusText() const {
    switch (_status) {
        case FeedbackCandidate::Status::Safe:
            return "SAFE";
        case FeedbackCandidate::Status::Warning:
            return "WARNING";
        case FeedbackCandidate::Status::Danger:
            return "DANGER";
        case FeedbackCandidate::Status::Feedback:
            return "FEEDBACK!";
        default:
            return "";
    }
}

void TrafficLightComponent::setStatus(FeedbackCandidate::Status status) {
    _status = status;
    repaint();
}

void TrafficLightComponent::setFrequency(float freq) {
    _frequency = freq;
    repaint();
}

void TrafficLightComponent::setRiskScore(float score) {
    _riskScore = juce::jlimit(0.0f, 1.0f, score);
    repaint();
}

} // namespace AudioCoPilot

