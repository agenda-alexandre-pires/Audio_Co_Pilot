#include "TransferFunctionComponent.h"
#include "../../UI/Colours.h"
#include <algorithm>
#include <cmath>

TransferFunctionComponent::TransferFunctionComponent()
{
    _magnitudeColour = juce::Colour(0xff4a9eff);  // Azul
    _phaseColour = juce::Colour(0xffff6b4a);      // Laranja
    _gridColour = juce::Colours::grey.withAlpha(0.3f);
    _backgroundColour = Colours::Background;
    
    // Configura ComboBox de resolução
    _resolutionLabel.setText("Resolution:", juce::dontSendNotification);
    _resolutionLabel.setColour(juce::Label::textColourId, Colours::TextPrimary);
    _resolutionLabel.attachToComponent(&_resolutionComboBox, true);
    addAndMakeVisible(_resolutionLabel);
    
    _resolutionComboBox.addItem("1/3 Octave", 1);
    _resolutionComboBox.addItem("1/6 Octave", 2);
    _resolutionComboBox.addItem("1/12 Octave", 3);
    _resolutionComboBox.addItem("1/24 Octave", 4);
    _resolutionComboBox.addItem("1/48 Octave", 5);
    _resolutionComboBox.setSelectedId(3, juce::dontSendNotification);  // Default: 1/12
    _resolutionComboBox.addListener(this);
    addAndMakeVisible(_resolutionComboBox);
    
    // Timer para atualização (30 FPS é suficiente para gráficos)
    startTimerHz(30);
}

void TransferFunctionComponent::paint(juce::Graphics& g)
{
    g.fillAll(_backgroundColour);
    
    auto bounds = getLocalBounds().toFloat().reduced(10.0f);
    
    // Reserva espaço para o controle de resolução no topo
    juce::ignoreUnused(bounds.removeFromTop(30.0f));
    bounds.removeFromTop(5.0f);  // Espaçamento
    
    if (_currentData.numBins == 0)
    {
        // Mensagem quando não há dados
        g.setColour(Colours::TextSecondary);
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 16.0f, juce::Font::plain)));
        
        juce::String message = "No Transfer Function Data\n(Requires 2 input channels)";
        if (_processor != nullptr)
        {
            auto debugInfo = _processor->getDebugInfo();
            message += "\n\nDebug Info:";
            message += "\nBuffer Ready: " + juce::String(debugInfo.bufferReady ? "Yes" : "No");
            message += "\nFrames Processed: " + juce::String(debugInfo.framesProcessed);
            message += "\nData Ready: " + juce::String(debugInfo.dataReady ? "Yes" : "No");
            message += "\nRef Level: " + juce::String(debugInfo.referenceLevel, 3);
            message += "\nMeas Level: " + juce::String(debugInfo.measurementLevel, 3);
        }
        
        g.drawText(message, bounds, juce::Justification::centred);
        return;
    }
    
    // Divide área: magnitude em cima, fase embaixo
    auto magnitudeBounds = bounds.removeFromTop(bounds.getHeight() * MAGNITUDE_HEIGHT_RATIO);
    auto phaseBounds = bounds;
    
    // Desenha grid e gráficos
    drawGrid(g, magnitudeBounds, true);
    drawMagnitudePlot(g, magnitudeBounds);
    
    drawGrid(g, phaseBounds, false);
    drawPhasePlot(g, phaseBounds);
    
    // Desenha tooltip se visível
    if (_tooltipInfo.visible)
    {
        drawTooltip(g);
    }
    
    // Labels dos eixos
    g.setColour(Colours::TextSecondary);
    g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 10.0f, juce::Font::plain)));
    
    // Label magnitude
    g.drawText("Magnitude (dB)", magnitudeBounds.withTrimmedLeft(5.0f).withHeight(15.0f),
               juce::Justification::centredLeft);
    
    // Label fase
    g.drawText("Phase (degrees)", phaseBounds.withTrimmedLeft(5.0f).withHeight(15.0f),
               juce::Justification::centredLeft);
    
    // Label frequência (embaixo)
    g.drawText("Frequency (Hz)", phaseBounds.withTrimmedBottom(15.0f).removeFromBottom(15.0f),
               juce::Justification::centred);
}

void TransferFunctionComponent::resized()
{
    // Posiciona controle de resolução no topo direito
    auto controlArea = getLocalBounds().reduced(10, 10);
    auto labelArea = controlArea.removeFromLeft(80);
    _resolutionLabel.setBounds(labelArea);
    _resolutionComboBox.setBounds(controlArea.removeFromLeft(150).withHeight(25));
    
    repaint();
}

void TransferFunctionComponent::timerCallback()
{
    if (_processor != nullptr)
    {
        auto newData = _processor->readTransferData();
        
        // Debug: verifica estado do processador
        auto debugInfo = _processor->getDebugInfo();
        
        // Atualiza apenas se houver dados válidos
        if (newData.numBins > 0)
        {
            _currentData = newData;
            repaint();
        }
        else if (debugInfo.bufferReady)
        {
            // Se o buffer está pronto mas não há dados, pode ser problema de cálculo
            // Força repaint para mostrar mensagem
            repaint();
        }
    }
}

void TransferFunctionComponent::setProcessor(TransferFunctionProcessor* processor)
{
    _processor = processor;
    
    // Sincroniza resolução atual
    if (_processor != nullptr)
    {
        auto currentRes = _processor->getOctaveResolution();
        int id = 3;  // Default 1/12
        switch (currentRes)
        {
            case OctaveResolution::ThirdOctave:
                id = 1;
                break;
            case OctaveResolution::SixthOctave:
                id = 2;
                break;
            case OctaveResolution::TwelfthOctave:
                id = 3;
                break;
            case OctaveResolution::TwentyFourthOctave:
                id = 4;
                break;
            case OctaveResolution::FortyEighthOctave:
                id = 5;
                break;
        }
        _resolutionComboBox.setSelectedId(id, juce::dontSendNotification);
    }
}

void TransferFunctionComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &_resolutionComboBox && _processor != nullptr)
    {
        int selectedId = _resolutionComboBox.getSelectedId();
        OctaveResolution resolution;
        
        switch (selectedId)
        {
            case 1:
                resolution = OctaveResolution::ThirdOctave;
                break;
            case 2:
                resolution = OctaveResolution::SixthOctave;
                break;
            case 3:
                resolution = OctaveResolution::TwelfthOctave;
                break;
            case 4:
                resolution = OctaveResolution::TwentyFourthOctave;
                break;
            case 5:
                resolution = OctaveResolution::FortyEighthOctave;
                break;
            default:
                resolution = OctaveResolution::TwelfthOctave;
                break;
        }
        
        _processor->setOctaveResolution(resolution);
    }
}

void TransferFunctionComponent::drawMagnitudePlot(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    if (_currentData.numBins == 0)
        return;
    
    // Área de plot (reduzida para labels)
    auto plotArea = bounds.reduced(50.0f, 25.0f);
    
    // Desenha linha vertical do cursor se tooltip estiver visível na área de magnitude
    if (_tooltipInfo.visible && bounds.contains(_tooltipInfo.position))
    {
        g.setColour(_magnitudeColour.withAlpha(0.5f));
        g.drawVerticalLine(static_cast<int>(_tooltipInfo.position.x), 
                          static_cast<int>(plotArea.getY()), 
                          static_cast<int>(plotArea.getBottom()));
    }
    
    // Cria path para a curva
    juce::Path magnitudePath;
    bool firstPoint = true;
    
    for (int i = 0; i < _currentData.numBins; ++i)
    {
        float freq = _currentData.frequencies[static_cast<size_t>(i)];
        
        // Pula frequências fora do range visível
        if (freq < FREQ_RANGE_MIN || freq > FREQ_RANGE_MAX)
            continue;
        
        float x = plotArea.getX() + freqToX(freq, plotArea.getWidth());
        float db = _currentData.magnitudeDb[static_cast<size_t>(i)];
        float y = plotArea.getY() + magnitudeToY(db, plotArea.getHeight());
        
        if (firstPoint)
        {
            magnitudePath.startNewSubPath(x, y);
            firstPoint = false;
        }
        else
        {
            magnitudePath.lineTo(x, y);
        }
    }
    
    // Desenha linha
    g.setColour(_magnitudeColour);
    g.strokePath(magnitudePath, juce::PathStrokeType(2.0f));
    
    // Desenha área preenchida (opcional, mais bonito)
    juce::Path filledPath = magnitudePath;
    filledPath.lineTo(plotArea.getRight(), plotArea.getBottom());
    filledPath.lineTo(plotArea.getX(), plotArea.getBottom());
    filledPath.closeSubPath();
    
    juce::ColourGradient gradient(
        _magnitudeColour.withAlpha(0.3f), plotArea.getTopLeft(),
        _magnitudeColour.withAlpha(0.05f), plotArea.getBottomLeft(),
        false
    );
    g.setGradientFill(gradient);
    g.fillPath(filledPath);
}

void TransferFunctionComponent::drawPhasePlot(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    if (_currentData.numBins == 0)
        return;
    
    // Área de plot
    auto plotArea = bounds.reduced(50.0f, 25.0f);
    
    // Cria path para a curva
    juce::Path phasePath;
    bool firstPoint = true;
    
    for (int i = 0; i < _currentData.numBins; ++i)
    {
        float freq = _currentData.frequencies[static_cast<size_t>(i)];
        
        if (freq < FREQ_RANGE_MIN || freq > FREQ_RANGE_MAX)
            continue;
        
        float x = plotArea.getX() + freqToX(freq, plotArea.getWidth());
        float phase = _currentData.phaseDegrees[static_cast<size_t>(i)];
        float y = plotArea.getY() + phaseToY(phase, plotArea.getHeight());
        
        if (firstPoint)
        {
            phasePath.startNewSubPath(x, y);
            firstPoint = false;
        }
        else
        {
            phasePath.lineTo(x, y);
        }
    }
    
    // Desenha linha
    g.setColour(_phaseColour);
    g.strokePath(phasePath, juce::PathStrokeType(2.0f));
}

void TransferFunctionComponent::drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds, bool isMagnitude)
{
    auto plotArea = bounds.reduced(50.0f, 25.0f);
    
    g.setColour(_gridColour);
    
    if (isMagnitude)
    {
        // Linhas horizontais (dB)
        for (int db = static_cast<int>(MAGNITUDE_RANGE_MIN); db <= static_cast<int>(MAGNITUDE_RANGE_MAX); db += 10)
        {
            float y = plotArea.getY() + magnitudeToY(static_cast<float>(db), plotArea.getHeight());
            g.drawHorizontalLine(static_cast<int>(y), plotArea.getX(), plotArea.getRight());
            
            // Label
            g.setColour(Colours::TextSecondary);
            g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 9.0f, juce::Font::plain)));
            g.drawText(juce::String(db), 
                       plotArea.withX(plotArea.getX() - 45.0f).withHeight(12.0f).withY(y - 6.0f),
                       juce::Justification::centredRight);
            g.setColour(_gridColour);
        }
    }
    else
    {
        // Linhas horizontais (fase)
        for (int phase = static_cast<int>(PHASE_RANGE_MIN); phase <= static_cast<int>(PHASE_RANGE_MAX); phase += 90)
        {
            float y = plotArea.getY() + phaseToY(static_cast<float>(phase), plotArea.getHeight());
            g.drawHorizontalLine(static_cast<int>(y), plotArea.getX(), plotArea.getRight());
            
            // Label
            g.setColour(Colours::TextSecondary);
            g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 9.0f, juce::Font::plain)));
            g.drawText(juce::String(phase), 
                       plotArea.withX(plotArea.getX() - 45.0f).withHeight(12.0f).withY(y - 6.0f),
                       juce::Justification::centredRight);
            g.setColour(_gridColour);
        }
    }
    
    // Linhas verticais (frequência) - mesma para ambos
    float freqs[] = { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    
    for (float freq : freqs)
    {
        if (freq < FREQ_RANGE_MIN || freq > FREQ_RANGE_MAX)
            continue;
        
        float x = plotArea.getX() + freqToX(freq, plotArea.getWidth());
        g.drawVerticalLine(static_cast<int>(x), plotArea.getY(), plotArea.getBottom());
        
        // Label (só na parte de baixo)
        if (!isMagnitude)
        {
            g.setColour(Colours::TextSecondary);
            g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 9.0f, juce::Font::plain)));
            
            juce::String label;
            if (freq >= 1000.0f)
                label = juce::String(freq / 1000.0f, 1) + "k";
            else
                label = juce::String(static_cast<int>(freq));
            
            g.drawText(label, 
                       static_cast<int>(x - 20), static_cast<int>(plotArea.getBottom() + 2), 40, 12,
                       juce::Justification::centred);
            g.setColour(_gridColour);
        }
    }
}

float TransferFunctionComponent::freqToX(float freq, float width) const
{
    // Escala logarítmica
    float logMin = std::log10(FREQ_RANGE_MIN);
    float logMax = std::log10(FREQ_RANGE_MAX);
    float logFreq = std::log10(std::max(FREQ_RANGE_MIN, std::min(FREQ_RANGE_MAX, freq)));
    
    float normalized = (logFreq - logMin) / (logMax - logMin);
    return normalized * width;
}

float TransferFunctionComponent::magnitudeToY(float db, float height) const
{
    db = juce::jlimit(MAGNITUDE_RANGE_MIN, MAGNITUDE_RANGE_MAX, db);
    float normalized = (db - MAGNITUDE_RANGE_MIN) / (MAGNITUDE_RANGE_MAX - MAGNITUDE_RANGE_MIN);
    return height * (1.0f - normalized);  // Invertido (0dB no topo)
}

float TransferFunctionComponent::phaseToY(float degrees, float height) const
{
    degrees = juce::jlimit(PHASE_RANGE_MIN, PHASE_RANGE_MAX, degrees);
    float normalized = (degrees - PHASE_RANGE_MIN) / (PHASE_RANGE_MAX - PHASE_RANGE_MIN);
    return height * (1.0f - normalized);  // Invertido (+180 no topo)
}

int TransferFunctionComponent::findBinForFrequency(float freq) const
{
    if (_currentData.numBins == 0)
        return 0;
    
    // Busca binário simples
    int closestBin = 0;
    float minDiff = std::abs(_currentData.frequencies[0] - freq);
    
    for (int i = 1; i < _currentData.numBins; ++i)
    {
        float diff = std::abs(_currentData.frequencies[static_cast<size_t>(i)] - freq);
        if (diff < minDiff)
        {
            minDiff = diff;
            closestBin = i;
        }
    }
    
    return closestBin;
}

float TransferFunctionComponent::xToFreq(float x, float width) const
{
    // Escala logarítmica inversa
    float logMin = std::log10(FREQ_RANGE_MIN);
    float logMax = std::log10(FREQ_RANGE_MAX);
    
    float normalized = x / width;
    float logFreq = logMin + normalized * (logMax - logMin);
    return std::pow(10.0f, logFreq);
}

void TransferFunctionComponent::mouseMove(const juce::MouseEvent& event)
{
    if (_currentData.numBins == 0)
    {
        _tooltipInfo.visible = false;
        repaint();
        return;
    }
    
    auto bounds = getLocalBounds().toFloat().reduced(10.0f);
    juce::ignoreUnused(bounds.removeFromTop(30.0f));
    bounds.removeFromTop(5.0f);
    
    auto magnitudeBounds = bounds.removeFromTop(bounds.getHeight() * MAGNITUDE_HEIGHT_RATIO);
    auto phaseBounds = bounds;
    
    auto magnitudePlotArea = magnitudeBounds.reduced(50.0f, 25.0f);
    auto phasePlotArea = phaseBounds.reduced(50.0f, 25.0f);
    
    juce::Point<float> mousePos = event.position;
    
    // Verifica se está sobre área de magnitude ou fase
    if (magnitudePlotArea.contains(mousePos) || phasePlotArea.contains(mousePos))
    {
        // Converte posição X para frequência
        float plotX = mousePos.x - magnitudePlotArea.getX();
        float freq = xToFreq(plotX, magnitudePlotArea.getWidth());
        
        // Limita frequência ao range válido
        freq = juce::jlimit(FREQ_RANGE_MIN, FREQ_RANGE_MAX, freq);
        
        // Encontra bin mais próximo
        int binIndex = findBinForFrequency(freq);
        
        if (binIndex >= 0 && binIndex < _currentData.numBins)
        {
            _tooltipInfo.visible = true;
            _tooltipInfo.frequency = freq;
            _tooltipInfo.magnitudeDb = _currentData.magnitudeDb[static_cast<size_t>(binIndex)];
            _tooltipInfo.phaseDegrees = _currentData.phaseDegrees[static_cast<size_t>(binIndex)];
            _tooltipInfo.position = mousePos;
            
            repaint();
        }
    }
    else
    {
        _tooltipInfo.visible = false;
        repaint();
    }
}

void TransferFunctionComponent::mouseExit(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    _tooltipInfo.visible = false;
    repaint();
}

void TransferFunctionComponent::drawTooltip(juce::Graphics& g)
{
    if (!_tooltipInfo.visible)
        return;
    
    // Formata texto do tooltip
    juce::String tooltipText;
    
    // Formata frequência
    if (_tooltipInfo.frequency >= 1000.0f)
        tooltipText += juce::String(_tooltipInfo.frequency / 1000.0f, 2) + " kHz";
    else
        tooltipText += juce::String(_tooltipInfo.frequency, 1) + " Hz";
    
    tooltipText += "\n";
    tooltipText += juce::String(_tooltipInfo.magnitudeDb, 2) + " dB";
    tooltipText += "\n";
    tooltipText += juce::String(_tooltipInfo.phaseDegrees, 1) + "°";
    
    // Calcula tamanho do texto
    juce::Font font(juce::FontOptions("Helvetica Neue", 12.0f, juce::Font::plain));
    g.setFont(font);
    
    // Calcula largura máxima das linhas
    float maxWidth = 0.0f;
    juce::StringArray lines;
    lines.addTokens(tooltipText, "\n", "");
    for (const auto& line : lines)
    {
        float width = static_cast<float>(font.getStringWidth(line));
        if (width > maxWidth)
            maxWidth = width;
    }
    
    float textHeight = font.getHeight();
    int numLines = lines.size();
    
    // Posiciona tooltip próximo ao cursor (evita sair da tela)
    float tooltipWidth = maxWidth + 20.0f;
    float tooltipHeight = textHeight * numLines + 10.0f;
    
    float tooltipX = _tooltipInfo.position.x + 15.0f;
    float tooltipY = _tooltipInfo.position.y - tooltipHeight / 2.0f;
    
    // Ajusta se sair da tela
    auto bounds = getLocalBounds().toFloat();
    if (tooltipX + tooltipWidth > bounds.getRight())
        tooltipX = _tooltipInfo.position.x - tooltipWidth - 15.0f;
    if (tooltipY + tooltipHeight > bounds.getBottom())
        tooltipY = bounds.getBottom() - tooltipHeight - 5.0f;
    if (tooltipY < bounds.getY())
        tooltipY = bounds.getY() + 5.0f;
    
    juce::Rectangle<float> tooltipRect(tooltipX, tooltipY, tooltipWidth, tooltipHeight);
    
    // Desenha fundo do tooltip
    g.setColour(juce::Colour(0xff2a2a2a).withAlpha(0.95f));
    g.fillRoundedRectangle(tooltipRect, 4.0f);
    
    // Desenha borda
    g.setColour(_magnitudeColour.withAlpha(0.8f));
    g.drawRoundedRectangle(tooltipRect, 4.0f, 1.5f);
    
    // Desenha texto
    g.setColour(Colours::TextPrimary);
    g.drawText(tooltipText, tooltipRect.reduced(10.0f, 5.0f), juce::Justification::centredLeft);
}