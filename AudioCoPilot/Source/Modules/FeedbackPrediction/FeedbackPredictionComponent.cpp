#include "FeedbackPredictionComponent.h"
#include "../../UI/Colours.h"
#include <algorithm>

namespace AudioCoPilot {

FeedbackPredictionComponent::FeedbackPredictionComponent(
    FeedbackPredictionProcessor& processor)
    : _processor(processor) {
    
    // Title
    _titleLabel.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 18.0f, juce::Font::bold)));
    _titleLabel.setColour(juce::Label::textColourId, Colours::TextPrimary);
    _titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(_titleLabel);
    
    // Traffic Light
    addAndMakeVisible(_trafficLight);
    
    // Buttons
    _enableButton.setClickingTogglesState(true);
    _enableButton.onClick = [this] { onEnableClicked(); };
    addAndMakeVisible(_enableButton);
    
    _learnButton.onClick = [this] { onLearnClicked(); };
    addAndMakeVisible(_learnButton);
    
    _clearButton.onClick = [this] { onClearClicked(); };
    addAndMakeVisible(_clearButton);
    
    // Frequency List
    _frequencyList.setModel(&_listModel);
    _frequencyList.setColour(juce::ListBox::backgroundColourId, 
                             juce::Colour(0xff1a1a1a));
    _frequencyList.setRowHeight(28);
    addAndMakeVisible(_frequencyList);
    
    // Timer para atualização
    startTimerHz(30);
}

FeedbackPredictionComponent::~FeedbackPredictionComponent() {
    stopTimer();
}

void FeedbackPredictionComponent::timerCallback() {
    updateUI();
}

void FeedbackPredictionComponent::updateUI() {
    const auto& detector = _processor.getDetector();
    const auto& candidates = detector.getCandidates();
    
    // Atualiza traffic light com pior status
    auto worstStatus = detector.getWorstStatus();
    _trafficLight.setStatus(worstStatus);
    _trafficLight.setFrequency(detector.getMostDangerousFrequency());
    
    // Encontra maior risk score
    float maxRisk = 0.0f;
    for (const auto& c : candidates) {
        maxRisk = std::max(maxRisk, c.riskScore);
    }
    _trafficLight.setRiskScore(maxRisk);
    
    // Atualiza lista de frequências
    _listModel.candidates.clear();
    for (const auto& c : candidates) {
        if (c.status != FeedbackCandidate::Status::Safe && c.magnitude > -60.0f) {
            _listModel.candidates.push_back(c);
        }
    }
    
    // Ordena por risco
    std::sort(_listModel.candidates.begin(), _listModel.candidates.end(),
              [](const auto& a, const auto& b) {
                  return a.riskScore > b.riskScore;
              });
    
    _frequencyList.updateContent();
    
    // Atualiza botões
    _enableButton.setToggleState(_processor.isEnabled(), juce::dontSendNotification);
    _enableButton.setButtonText(_processor.isEnabled() ? "Enabled" : "Disabled");
    
    if (_processor.isLearning()) {
        _learnButton.setButtonText("Learning...");
    } else {
        _learnButton.setButtonText("Learn");
    }
}

void FeedbackPredictionComponent::paint(juce::Graphics& g) {
    g.fillAll(Colours::Background);
    
    // Linha divisória entre as duas colunas
    auto bounds = getLocalBounds();
    bounds.removeFromTop(60); // Pula o título
    g.setColour(juce::Colour(0xff333333));
    g.drawVerticalLine(bounds.getWidth() / 2, 
                      static_cast<float>(bounds.getY()), 
                      static_cast<float>(bounds.getBottom()));
    
    // Label para lista de frequências (coluna direita)
    auto rightColumn = bounds.removeFromRight(bounds.getWidth() / 2);
    g.setColour(Colours::TextSecondary);
    g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 14.0f, juce::Font::bold)));
    g.drawText("Frequencies at Risk", 
               rightColumn.getX() + 20, 
               rightColumn.getY() + 5, 
               200, 20,
               juce::Justification::centredLeft);
}

void FeedbackPredictionComponent::resized() {
    auto bounds = getLocalBounds();
    
    // Title no topo
    _titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(20);
    
    // Divide a tela em duas colunas iguais
    auto leftColumn = bounds.removeFromLeft(bounds.getWidth() / 2);
    auto rightColumn = bounds;
    
    // ===== COLUNA ESQUERDA: Traffic Light e Botões (centralizados) =====
    int leftPadding = 40;
    int rightPadding = 40;
    auto leftContent = leftColumn.reduced(leftPadding, 0);
    
    // Centraliza conteúdo verticalmente
    int trafficLightHeight = 250;
    int buttonsHeight = 120;
    int spacing = 30;
    int totalHeight = trafficLightHeight + spacing + buttonsHeight;
    int startY = (leftContent.getHeight() - totalHeight) / 2;
    
    // Traffic Light centralizado horizontalmente e verticalmente
    int trafficLightWidth = juce::jmin(300, leftContent.getWidth() - 40);
    int trafficLightX = leftContent.getX() + (leftContent.getWidth() - trafficLightWidth) / 2;
    _trafficLight.setBounds(trafficLightX, 
                           leftContent.getY() + startY, 
                           trafficLightWidth, 
                           trafficLightHeight);
    
    // Botões centralizados abaixo do traffic light
    int buttonWidth = 140;
    int buttonX = leftContent.getX() + (leftContent.getWidth() - buttonWidth) / 2;
    int buttonY = leftContent.getY() + startY + trafficLightHeight + spacing;
    
    _enableButton.setBounds(buttonX, buttonY, buttonWidth, 32);
    _learnButton.setBounds(buttonX, buttonY + 38, buttonWidth, 32);
    _clearButton.setBounds(buttonX, buttonY + 76, buttonWidth, 32);
    
    // ===== COLUNA DIREITA: Lista de Frequências =====
    auto rightContent = rightColumn.reduced(20, 20);
    
    // Label para a lista
    juce::Rectangle<int> listBounds = rightContent;
    _frequencyList.setBounds(listBounds);
}

void FeedbackPredictionComponent::onEnableClicked() {
    _processor.setEnabled(_enableButton.getToggleState());
}

void FeedbackPredictionComponent::onLearnClicked() {
    if (_processor.isLearning()) {
        _processor.stopLearning();
    } else {
        _processor.startLearning();
    }
}

void FeedbackPredictionComponent::onClearClicked() {
    _processor.clearLearned();
}

void FeedbackPredictionComponent::FrequencyListModel::paintListBoxItem(
    int row, juce::Graphics& g, int width, int height, bool selected) {
    
    if (row < 0 || row >= static_cast<int>(candidates.size())) return;
    
    const auto& c = candidates[static_cast<size_t>(row)];
    
    // Background baseado no status
    juce::Colour bgColour;
    switch (c.status) {
        case FeedbackCandidate::Status::Warning:
            bgColour = juce::Colour(0xff3d3d00); break;
        case FeedbackCandidate::Status::Danger:
            bgColour = juce::Colour(0xff3d1a00); break;
        case FeedbackCandidate::Status::Feedback:
            bgColour = juce::Colour(0xff3d0000); break;
        default:
            bgColour = juce::Colour(0xff1a1a1a); break;
    }
    
    if (selected) bgColour = bgColour.brighter(0.2f);
    g.fillAll(bgColour);
    
    // Frequência
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 14.0f, juce::Font::bold)));
    
    juce::String freqText;
    if (c.frequency >= 1000.0f) {
        freqText = juce::String(c.frequency / 1000.0f, 2) + " kHz";
    } else {
        freqText = juce::String(static_cast<int>(c.frequency)) + " Hz";
    }
    g.drawText(freqText, 8, 0, 80, height, juce::Justification::centredLeft);
    
    // Status
    juce::Colour statusColour;
    juce::String statusText;
    switch (c.status) {
        case FeedbackCandidate::Status::Warning:
            statusColour = juce::Colour(0xffffcc00);
            statusText = "WARN"; break;
        case FeedbackCandidate::Status::Danger:
            statusColour = juce::Colour(0xffff6600);
            statusText = "DNGR"; break;
        case FeedbackCandidate::Status::Feedback:
            statusColour = juce::Colour(0xffff0000);
            statusText = "FB!"; break;
        default:
            statusColour = juce::Colours::grey;
            statusText = "OK"; break;
    }
    
    g.setColour(statusColour);
    g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 12.0f, juce::Font::bold)));
    g.drawText(statusText, 90, 0, 50, height, juce::Justification::centred);
    
    // Risk bar ou Loop Gain
    float barX = 145.0f;
    float barWidth = static_cast<float>(width) - barX - 10.0f;
    
    // Se tiver referência (loopGain > -90), mostra o valor em dB
    if (c.loopGain > -90.0f) {
        g.setColour(Colours::TextSecondary);
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 12.0f, juce::Font::plain)));
        
        juce::String gainText = "Gain: " + juce::String(c.loopGain > 0 ? "+" : "") + 
                               juce::String(c.loopGain, 1) + " dB";
        
        g.drawText(gainText, static_cast<int>(barX), 0, static_cast<int>(barWidth), height, 
                   juce::Justification::centredLeft);
    } 
    else {
        // Fallback para barra de risco
        float barHeight = 10.0f;
        float barY = (static_cast<float>(height) - barHeight) / 2.0f;
        
        g.setColour(juce::Colour(0xff333333));
        g.fillRoundedRectangle(barX, barY, barWidth, barHeight, 3.0f);
        
        g.setColour(statusColour);
        g.fillRoundedRectangle(barX, barY, barWidth * c.riskScore, barHeight, 3.0f);
    }
}

} // namespace AudioCoPilot

