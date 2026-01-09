# Audio Co-Pilot

**Aplicação profissional de análise acústica para engenheiros de som ao vivo.**

Desenvolvido por [Emerson Porfa Audio](https://www.emersonporfaaudio.com.br)

## Visão Geral

Audio Co-Pilot é uma ferramenta de análise acústica em tempo real projetada para engenheiros de som profissionais que trabalham com shows ao vivo. O software oferece:

- **Transfer Function Analysis** - Análise de resposta de frequência dual-channel
- **Anti-Masking Matrix** - Detecção de mascaramento psicoacústico
- **Feedback Prediction** - Prevenção de microfonia em tempo real

## Requisitos

- macOS 13.0 ou superior
- Apple Silicon (M1/M2/M3/M4)
- Interface de áudio compatível com CoreAudio

## Build

```bash
cd ~/Desktop/AudioCoPilot/AudioCoPilot
cmake -B Builds -G Xcode
cmake --build Builds --config Release
```

## Estrutura

```
Source/
├── Core/           # Engine de áudio e gerenciamento de dispositivos
├── UI/             # Interface gráfica
├── DSP/            # Processamento de sinais
└── Modules/        # Módulos de análise (Transfer Function, Anti-Masking, Feedback)
```

## Licença

Copyright © 2024 Emerson Porfa Audio. Todos os direitos reservados.

