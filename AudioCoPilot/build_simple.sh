#!/bin/bash

echo "=========================================="
echo "COMPILANDO VERSÃO SIMPLIFICADA DO AUDIO CO-PILOT"
echo "=========================================="

# Diretório do projeto
PROJECT_DIR="/Users/emersonporfa/Desktop/AudioCoPilot/AudioCoPilot"
BUILD_DIR="$PROJECT_DIR/build_simple"

# Cria diretório de build se não existir
mkdir -p "$BUILD_DIR"

# Entra no diretório
cd "$BUILD_DIR"

# Configura CMake
echo "Configurando CMake..."
cmake "$PROJECT_DIR" -DCMAKE_BUILD_TYPE=Debug

# Compila
echo "Compilando..."
make -j4

# Verifica se compilou
if [ $? -eq 0 ]; then
    echo "=========================================="
    echo "COMPILAÇÃO BEM-SUCEDIDA!"
    echo "=========================================="
    echo ""
    echo "Para executar:"
    echo "cd $BUILD_DIR"
    echo "./AudioCoPilot_Simple_artefacts/Debug/Audio\ Co-Pilot\ \(Simple\).app/Contents/MacOS/Audio\ Co-Pilot\ \(Simple\)"
    echo ""
    echo "OU use o script: ./run_simple.sh"
else
    echo "=========================================="
    echo "ERRO NA COMPILAÇÃO!"
    echo "=========================================="
    exit 1
fi