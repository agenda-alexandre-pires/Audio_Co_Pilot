# RESUMO DO CONTEXTO - AudioCoPilot

## PROBLEMA ORIGINAL
- App travava completamente ao mudar de módulo
- Deadlock entre threads (audio thread vs UI thread)
- Seletor de dispositivos causava travamento
- Áudio não iniciava no módulo Meters
- Ícone de microfone não aparecia no macOS (sem permissão)

## CORREÇÕES IMPLEMENTADAS

### 1. Thread Safety no DeviceManager
- Adicionado `std::mutex` para proteger operações no `AudioDeviceManager`
- Métodos `getAvailableDevices()`, `getCurrentDevice()`, `setDevice()` agora são thread-safe
- Proteção contra mudanças concorrentes de dispositivo

### 2. Arquitetura Simplificada
- Áudio inicializado **APENAS UMA VEZ** quando necessário
- DeviceManager criado na inicialização, mas áudio só inicia quando:
  - Dispositivo é selecionado no DeviceSelector (callback `onAudioShouldEnable`)
  - OU quando sai do módulo Meters pela primeira vez
- Nenhuma reinicialização ao trocar entre módulos

### 3. MainComponent Simplificado
- Removido carregamento gradual problemático
- Componentes criados imediatamente na inicialização
- Callback configurado para iniciar áudio quando dispositivo é selecionado
- Método `initializeAudioOnce()` - garante que áudio só inicia uma vez

### 4. Permissão de Microfone
- **CMakeLists.txt linha 44**: `MICROPHONE_PERMISSION_ENABLED TRUE`
- Removida versão Simple que causava conflitos
- App deve solicitar permissão na primeira execução

## ESTRUTURA ATUAL

### Arquivos Principais
- `Source/MainComponent.cpp` - Versão simplificada e estável
- `Source/MainComponent.h` - Header com métodos simplificados
- `Source/Core/DeviceManager.cpp` - Thread-safe com mutex
- `Source/Core/AudioEngine.cpp` - Logs detalhados adicionados
- `Source/UI/Components/DeviceSelector.cpp` - Callback para iniciar áudio

### Fluxo de Inicialização
1. App abre → MainComponent criado
2. DeviceManager criado (mas áudio NÃO inicializado)
3. DeviceSelector configurado com callback `onAudioShouldEnable`
4. Quando usuário seleciona dispositivo → `initializeAudioOnce()` é chamado
5. DeviceManager::initialise() → AudioEngine criado → AudioEngine::start()
6. Áudio roda continuamente até app fechar

### Troca de Módulos
- Meters → Outro módulo: Inicializa áudio se não estiver rodando
- Entre módulos não-Meters: Apenas habilita/desabilita processadores
- Volta para Meters: Desabilita processadores, mantém áudio rodando

## PROBLEMA ATUAL (NÃO RESOLVIDO)

### Sintomas
- Dispositivo selecionado mas áudio não inicia
- Status bar mostra "dispositivo sem acionar"
- Ícone de microfone NÃO aparece no menu do macOS
- macOS não solicita permissão de microfone

### Possíveis Causas
1. **Permissão não está sendo solicitada** - Mesmo com `MICROPHONE_PERMISSION_ENABLED TRUE`
2. **DeviceManager::initialise() não está realmente abrindo o dispositivo**
3. **macOS pode estar bloqueando silenciosamente** - App precisa ser executado de forma específica
4. **Info.plist pode não ter a chave correta** - JUCE deveria gerar, mas pode estar faltando

### Próximos Passos Sugeridos
1. Verificar se `Info.plist` tem `NSMicrophoneUsageDescription`
2. Testar executar app diretamente do terminal (não do Xcode)
3. Verificar logs do sistema: `log show --predicate 'process == "Audio Co-Pilot"' --last 5m`
4. Adicionar solicitação explícita de permissão no código
5. Verificar se `DeviceManager::initialise()` está realmente abrindo o dispositivo

## COMANDOS ÚTEIS

### Compilar
```bash
cd /Users/emersonporfa/Desktop/AudioCoPilot/AudioCoPilot
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug -j4
```

### Executar
```bash
./build/AudioCoPilot_artefacts/Debug/"Audio Co-Pilot.app"/Contents/MacOS/"Audio Co-Pilot"
```

### Verificar Permissões
```bash
# Ver permissões do app
tccutil reset Microphone com.emersonporfaaudio.audiocopilot

# Ver logs do sistema
log show --predicate 'process == "Audio Co-Pilot"' --last 5m
```

## ARQUIVOS MODIFICADOS

### Core
- `Source/Core/DeviceManager.h` - Adicionado `std::mutex _deviceMutex` e `std::atomic<bool> _changingDevice`
- `Source/Core/DeviceManager.cpp` - Todos os métodos protegidos com mutex
- `Source/Core/AudioEngine.cpp` - Logs detalhados em `start()` e `stop()`

### UI
- `Source/UI/Components/DeviceSelector.h` - Adicionado callback `onAudioShouldEnable`
- `Source/UI/Components/DeviceSelector.cpp` - Chama callback quando dispositivo é selecionado
- `Source/UI/Components/HeaderBar.h` - Método `getDeviceSelector()` para acesso

### Main
- `Source/MainComponent.h` - Simplificado, removido carregamento gradual
- `Source/MainComponent.cpp` - Versão completamente reescrita, estável
- `CMakeLists.txt` - `MICROPHONE_PERMISSION_ENABLED TRUE` (linha 44)

## REGRAS DO PROJETO
- Framework: JUCE / C++20
- Sistema Operacional: macOS (Core Audio)
- Real-time Safety: Proibido alocar memória no `processBlock`
- DSP: Use `juce::dsp` para performance
- Thread Safety: Lock-free quando possível, mutex quando necessário

## STATUS ATUAL
✅ Deadlock resolvido
✅ Troca de módulos funcionando
✅ Thread safety implementado
✅ Arquitetura simplificada
❌ Permissão de microfone não está funcionando
❌ Áudio não inicia quando dispositivo é selecionado
❌ Ícone de microfone não aparece no macOS