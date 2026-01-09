#include "MeterBridge.h"
#include "../Colours.h"

MeterBridge::MeterBridge()
{
    // Coeficiente de decay: 200ms para cair a 36.8% do valor
    // refreshRate = 60 fps (timer da UI)
    // decayTime = 0.2 segundos
    _decayCoeff = 1.0f - std::exp(-1.0f / (60.0f * 0.2f));
    
    // Configura botão de clear peak hold
    _clearPeakHoldButton.setButtonText("Clear Peak Hold");
    _clearPeakHoldButton.addListener(this);
    _clearPeakHoldButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
    _clearPeakHoldButton.setColour(juce::TextButton::textColourOffId, Colours::TextPrimary);
    addAndMakeVisible(_clearPeakHoldButton);
}

void MeterBridge::paint(juce::Graphics& g)
{
    g.fillAll(Colours::Background);
    
    if (_levels.empty())
    {
        // Mensagem quando não há áudio
        g.setColour(Colours::TextSecondary);
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 16.0f, juce::Font::plain)));
        g.drawText("No Audio Input", getLocalBounds(), juce::Justification::centred);
        return;
    }

    auto bounds = getLocalBounds().toFloat().reduced(20.0f);
    
    // Reserva espaço para o botão no topo
    bounds.removeFromTop(35.0f);
    int numChannels = static_cast<int>(_levels.size());
    
    // Limita a 64 canais visíveis
    numChannels = std::min(numChannels, 64);
    
    if (numChannels == 0)
        return;
    
    // Define tamanho mínimo e máximo do medidor
    const float minMeterWidth = 25.0f;
    const float maxMeterWidth = 80.0f;
    const float spacing = 8.0f;
    const float padding = 10.0f;
    
    // Calcula quantos medidores cabem em uma linha
    float availableWidth = bounds.getWidth() - padding * 2.0f;
    int metersPerRow = static_cast<int>((availableWidth + spacing) / (minMeterWidth + spacing));
    metersPerRow = std::max(1, metersPerRow); // Pelo menos 1 por linha
    
    // Calcula largura ideal do medidor baseado no número de canais e espaço disponível
    float meterWidth;
    if (numChannels <= metersPerRow)
    {
        // Todos cabem em uma linha - distribui uniformemente
        if (numChannels == 1)
        {
            meterWidth = std::min(maxMeterWidth, availableWidth);
        }
        else
        {
            meterWidth = (availableWidth - spacing * static_cast<float>(numChannels - 1)) / static_cast<float>(numChannels);
            meterWidth = juce::jlimit(minMeterWidth, maxMeterWidth, meterWidth);
        }
    }
    else
    {
        // Precisa de múltiplas linhas - usa largura fixa baseada no espaço disponível
        meterWidth = (availableWidth - spacing * static_cast<float>(metersPerRow - 1)) / static_cast<float>(metersPerRow);
        meterWidth = juce::jlimit(minMeterWidth, maxMeterWidth, meterWidth);
    }
    
    // Calcula número de linhas necessárias
    int numRows = (numChannels + metersPerRow - 1) / metersPerRow; // Arredonda para cima
    float totalHeight = bounds.getHeight();
    float rowHeight = totalHeight / static_cast<float>(numRows);
    
    // Desenha medidores em grade
    for (int i = 0; i < numChannels; ++i)
    {
        int row = i / metersPerRow;
        int col = i % metersPerRow;
        
        float x = bounds.getX() + padding + col * (meterWidth + spacing);
        float y = bounds.getY() + row * rowHeight;
        float height = rowHeight - 4.0f; // Pequeno espaçamento entre linhas
        
        auto meterBounds = juce::Rectangle<float>(x, y, meterWidth, height);
        drawMeter(g, meterBounds, _levels[static_cast<size_t>(i)].peakLevel,
                  _levels[static_cast<size_t>(i)].rmsLevel,
                  _levels[static_cast<size_t>(i)].clipping, i + 1);
    }
}

void MeterBridge::resized()
{
    // Posiciona botão Clear Peak Hold no topo direito
    auto bounds = getLocalBounds().reduced(10, 10);
    _clearPeakHoldButton.setBounds(bounds.removeFromTop(25).removeFromRight(150));
    
    repaint();
}

void MeterBridge::setLevels(const std::vector<ChannelLevels>& newLevels)
{
    // Redimensiona arrays de display se necessário
    if (_displayPeak.size() != newLevels.size())
    {
        _displayPeak.resize(newLevels.size(), 0.0f);
        _displayRms.resize(newLevels.size(), 0.0f);
        _displayClip.resize(newLevels.size(), false);
        _peakHoldValues.resize(newLevels.size(), -100.0f);  // Inicializa em -100dB (silêncio)
    }
    
    // Processa cada canal
    for (size_t i = 0; i < newLevels.size(); ++i)
    {
        float newPeak = newLevels[i].peakLevel;  // Valor LINEAR (0.0 a 1.0+)
        float newRms = newLevels[i].rmsLevel;    // Valor LINEAR
        
        // PEAK: Attack instantâneo, decay suave
        if (newPeak > _displayPeak[i])
        {
            _displayPeak[i] = newPeak;
        }
        else
        {
            // Decay em direção ao novo valor (que é 0 quando não há som)
            _displayPeak[i] += (newPeak - _displayPeak[i]) * _decayCoeff;
        }
        
        // RMS: Mesmo padrão
        if (newRms > _displayRms[i])
        {
            _displayRms[i] = newRms;
        }
        else
        {
            _displayRms[i] += (newRms - _displayRms[i]) * _decayCoeff;
        }
        
        // Clipping: mantém true por alguns frames se detectado
        if (newLevels[i].clipping)
            _displayClip[i] = true;
    }
    
    // Atualiza peak hold (mantém o máximo)
    for (size_t i = 0; i < newLevels.size(); ++i)
    {
        float peakDb = (_displayPeak[i] > 1e-10f) 
            ? 20.0f * std::log10(_displayPeak[i]) 
            : -100.0f;
        
        // Se o novo peak é maior que o peak hold atual, atualiza
        if (peakDb > _peakHoldValues[i])
        {
            _peakHoldValues[i] = peakDb;
        }
    }
    
    // Prepara _levels com valores em dB para o desenho
    _levels.resize(newLevels.size());
    for (size_t i = 0; i < newLevels.size(); ++i)
    {
        // Converte LINEAR para dB só agora, para desenhar
        _levels[i].peakLevel = (_displayPeak[i] > 1e-10f) 
            ? 20.0f * std::log10(_displayPeak[i]) 
            : -100.0f;
        _levels[i].rmsLevel = (_displayRms[i] > 1e-10f) 
            ? 20.0f * std::log10(_displayRms[i]) 
            : -100.0f;
        _levels[i].clipping = _displayClip[i];
    }
    
    repaint();
}

void MeterBridge::clearPeakHold()
{
    std::fill(_peakHoldValues.begin(), _peakHoldValues.end(), -100.0f);
    repaint();
}

void MeterBridge::buttonClicked(juce::Button* button)
{
    if (button == &_clearPeakHoldButton)
    {
        clearPeakHold();
    }
}

void MeterBridge::drawMeter(juce::Graphics& g, juce::Rectangle<float> bounds,
                             float peakDB, float rmsDB, bool clipping, int channelNum)
{
    // Fundo do meter
    g.setColour(Colours::MeterBackground);
    g.fillRoundedRectangle(bounds, 2.0f);
    
    // Borda
    g.setColour(Colours::MeterOutline);
    g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
    
    float meterHeight = bounds.getHeight() - 20.0f; // Espaço para número do canal
    auto meterArea = bounds.withTrimmedBottom(20.0f).reduced(2.0f);
    
    // Calcula posições Y (0dB no topo, -60dB embaixo)
    float rmsY = dbToY(rmsDB, meterArea.getHeight());
    float peakY = dbToY(peakDB, meterArea.getHeight());
    
    // Barra RMS completa (de baixo até o nível RMS)
    // A barra vai de meterArea.getBottom() até meterArea.getY() + rmsY
    auto rmsBarRect = juce::Rectangle<float>(
        meterArea.getX(),
        meterArea.getY() + rmsY,
        meterArea.getWidth(),
        meterArea.getBottom() - (meterArea.getY() + rmsY)
    );
    
    // Gradiente vertical para RMS (verde embaixo, vermelho em cima)
    juce::ColourGradient rmsGradient(
        Colours::MeterGreen, rmsBarRect.getBottomLeft(),
        Colours::MeterRed, rmsBarRect.getTopLeft(),
        false
    );
    rmsGradient.addColour(0.3f, Colours::MeterGreen);   // -18dB: verde
    rmsGradient.addColour(0.6f, Colours::MeterYellow);  // -12dB: amarelo
    rmsGradient.addColour(0.8f, Colours::MeterOrange);  // -6dB: laranja
    
    g.setGradientFill(rmsGradient);
    g.fillRoundedRectangle(rmsBarRect.reduced(1.0f, 0.0f), 1.0f);
    
    // Barra de Peak Hold (mais fina, à direita, de baixo até o peak)
    if (peakDB > rmsDB) // Só mostra se o peak for maior que RMS
    {
        float peakBarWidth = meterArea.getWidth() * 0.3f; // 30% da largura
        auto peakBarRect = juce::Rectangle<float>(
            meterArea.getRight() - peakBarWidth - 1.0f,
            meterArea.getY() + peakY,
            peakBarWidth,
            meterArea.getBottom() - (meterArea.getY() + peakY)
        );
        
        // Cor do peak baseada no nível
        juce::Colour peakColour = getColourForLevel(peakDB);
        if (clipping)
            peakColour = Colours::Alert;
        
        g.setColour(peakColour);
        g.fillRoundedRectangle(peakBarRect.reduced(0.5f, 0.0f), 0.5f);
        
        // Linha indicadora no topo do peak
        g.setColour(peakColour.brighter(0.3f));
        g.fillRect(peakBarRect.getX(), peakBarRect.getY(), peakBarRect.getWidth(), 2.0f);
    }
    
    // Desenha linha de Peak Hold máximo (se houver e for maior que o peak atual)
    if (channelNum > 0 && channelNum <= static_cast<int>(_peakHoldValues.size()))
    {
        float peakHoldDb = _peakHoldValues[static_cast<size_t>(channelNum - 1)];
        if (peakHoldDb > peakDB && peakHoldDb > -60.0f)  // Só mostra se for maior que o peak atual e acima de -60dB
        {
            float peakHoldY = dbToY(peakHoldDb, meterArea.getHeight());
            
            // Linha horizontal branca tracejada indicando o peak hold máximo
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            float dashLength = 4.0f;
            float x = meterArea.getX();
            while (x < meterArea.getRight())
            {
                float segmentEnd = std::min(x + dashLength, meterArea.getRight());
                g.fillRect(x, meterArea.getY() + peakHoldY - 1.0f, segmentEnd - x, 2.0f);
                x += dashLength * 2.0f;  // Pula o espaço entre traços
            }
        }
    }
    
    // Clip indicator no topo
    if (clipping)
    {
        auto clipRect = bounds.withHeight(6.0f).reduced(2.0f, 0.0f);
        g.setColour(Colours::Alert);
        g.fillRect(clipRect);
    }
    
    // Número do canal
    g.setColour(Colours::TextSecondary);
    g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 10.0f, juce::Font::plain)));
    g.drawText(juce::String(channelNum),
               bounds.withTop(bounds.getBottom() - 18.0f).withHeight(16.0f).toNearestInt(),
               juce::Justification::centred);
}

float MeterBridge::dbToY(float db, float height) const
{
    // Range: 0dB no topo, -60dB embaixo
    const float minDB = -60.0f;
    const float maxDB = 0.0f;
    
    db = juce::jlimit(minDB, maxDB, db);
    
    // Escala logarítmica
    float normalized = (db - minDB) / (maxDB - minDB);
    return height * (1.0f - normalized);
}

juce::Colour MeterBridge::getColourForLevel(float db) const
{
    if (db > -3.0f)
        return Colours::MeterRed;
    else if (db > -12.0f)
        return Colours::MeterOrange;
    else if (db > -24.0f)
        return Colours::MeterYellow;
    else
        return Colours::MeterGreen;
}

