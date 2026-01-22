# Problema: Transfer Function - Delay Detection Lento

## Contexto
- **App**: Audio Co-Pilot (JUCE 7.0+, C++20, macOS)
- **Módulo**: Transfer Function (H(f) = Y(f)/X(f))
- **Problema**: Delay detection demora 30+ segundos, enquanto Smaart faz em ~2 segundos

## Comportamento Esperado (Smaart)
1. Usuário seleciona canais REF e MEAS
2. Delay aparece em ~2 segundos
3. Usuário clica "Insert Delay"
4. Gráficos aparecem estáveis e fixos (não tremem)

## Comportamento Atual (Nosso App)
1. Usuário seleciona canais REF e MEAS
2. Delay demora 30+ segundos para aparecer
3. Gráficos aparecem aos poucos e tremem
4. Linhas não ficam fixas como no Smaart

## Implementação Atual

### Arquivos Principais
- `Source/Core/TransferFunction/TFProcessor.h` - Declaração do processador
- `Source/Core/TransferFunction/TFProcessor.cpp` - Implementação
- `Source/Core/TransferFunction/TFController.cpp` - Controlador de áudio

### Configuração Atual
- **FFT Size**: 16384 (power of 2)
- **Overlap**: 75% (hop = 4096 samples @ 48kHz)
- **Averaging Time**: 1.5s (exponential averaging)
- **Smoothing**: 1/12 octave
- **Delay Detection**: GCC-PHAT usando espectro instantâneo (X, Y)

### Pipeline Atual (processFrame)
1. `updateAverages()` - Atualiza Gxx, Gyy, Gxy e calcula H = Gxy/(Gxx+eps)
2. `estimateDelay()` - GCC-PHAT a cada 4 frames quando buscando
3. `applyDelayCompensation()` - Aplica apenas quando `delayLocked == true`
4. `applySmoothing()` - Smoothing 1/12 octave
5. `unwrapPhase()` - Unwrap de fase
6. `extractMagnitudeAndPhase()` - Extrai magnitude e fase

### GCC-PHAT Implementation
```cpp
void TFProcessor::estimateDelay()
{
    // Usa espectro instantâneo X e Y (não averaged)
    // Calcula C_phat = (Y * conj(X)) / |Y * conj(X)|
    // IFFT para obter correlação no tempo
    // Encontra pico para determinar delay
    // Smoothing e locking logic
}
```

## Problemas Identificados

### 1. Delay Update Frequency
- Atual: A cada 4 frames quando buscando
- Problema: Pode ser muito lento para resposta rápida

### 2. Delay Locking
- Atual: 5 updates estáveis com threshold 0.05ms
- Problema: Pode ser muito conservador

### 3. Delay Compensation
- Atual: Aplica apenas quando `delayLocked == true`
- Problema: Gráficos não aparecem até delay estar locked

### 4. Averaging Time
- Atual: 1.5s
- Problema: Pode ser muito lento para resposta inicial

## Perguntas para Claude/Assistente

1. **Como o Smaart detecta delay tão rápido?**
   - Usa janela menor para delay detection?
   - Usa método diferente de GCC-PHAT?
   - Aplica delay compensation antes do lock?

2. **Otimização de GCC-PHAT**
   - Devo usar FFT menor para delay detection?
   - Devo usar menos bins de frequência?
   - Devo usar método diferente (cross-correlation direta)?

3. **Pipeline de Processamento**
   - Devo calcular delay ANTES do averaging?
   - Devo aplicar delay compensation mesmo sem lock?
   - Como balancear velocidade vs estabilidade?

4. **Gráficos Estáveis**
   - Como fazer linhas ficarem fixas (não tremem)?
   - Devo usar double-buffering?
   - Devo reduzir frequência de updates da UI?

## Código Relevante

### TFProcessor::processFrame()
```cpp
void TFProcessor::processFrame()
{
    // 1. Update averages
    updateAverages();
    
    // 2. Estimate delay (every 4 frames when searching)
    delayUpdateCounter++;
    if (delayUpdateCounter >= 4 && !delayLocked) {
        estimateDelay();
    }
    
    // 3. Apply delay compensation (only when locked)
    if (delayLocked) {
        applyDelayCompensation();
    }
    
    // 4. Smoothing, unwrap, extract
    applySmoothing();
    unwrapPhase();
    extractMagnitudeAndPhase();
}
```

### TFProcessor::estimateDelay()
- Usa GCC-PHAT com espectro instantâneo
- IFFT para correlação temporal
- Encontra pico e converte para delay
- Smoothing e locking logic

## Objetivo Final
- Delay aparece em ~2 segundos após selecionar canais
- Gráficos aparecem estáveis e fixos (como Smaart)
- Linhas não tremem durante atualização
