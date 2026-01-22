#!/bin/bash

# Script para verificar configuração da API Key do Claude no Cursor

echo "🔍 Verificando configuração da API Key do Claude no Cursor..."
echo ""

# Cores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Verificar se o Cursor está instalado
CURSOR_PATH="$HOME/Library/Application Support/Cursor"
if [ ! -d "$CURSOR_PATH" ]; then
    echo -e "${RED}❌ Cursor não encontrado em: $CURSOR_PATH${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Cursor encontrado${NC}"
echo ""

# Verificar arquivos de configuração
echo "📁 Verificando arquivos de configuração..."

SETTINGS_FILE="$CURSOR_PATH/User/settings.json"
if [ -f "$SETTINGS_FILE" ]; then
    echo -e "${GREEN}✅ settings.json encontrado${NC}"
    
    # Verificar se "Override OpenAI Base URL" está desativado
    if grep -q "overrideOpenAIBaseURL" "$SETTINGS_FILE"; then
        echo -e "${YELLOW}⚠️  'Override OpenAI Base URL' encontrado no settings.json${NC}"
        echo "   Certifique-se de que está DESATIVADO nas configurações do Cursor"
    else
        echo -e "${GREEN}✅ 'Override OpenAI Base URL' não configurado (bom)${NC}"
    fi
else
    echo -e "${YELLOW}⚠️  settings.json não encontrado${NC}"
fi

echo ""

# Verificar storage
STORAGE_FILE="$CURSOR_PATH/User/globalStorage/storage.json"
if [ -f "$STORAGE_FILE" ]; then
    echo -e "${GREEN}✅ storage.json encontrado${NC}"
    
    # Tentar verificar se há configurações de modelo
    if python3 -c "import json; data = json.load(open('$STORAGE_FILE')); print('Model configs found' if any('model' in k.lower() for k in data.keys()) else 'No model configs')" 2>/dev/null; then
        echo "   (Configurações de modelo podem estar presentes)"
    fi
else
    echo -e "${YELLOW}⚠️  storage.json não encontrado${NC}"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📋 CHECKLIST DE CONFIGURAÇÃO:"
echo ""
echo "1. Abra o Cursor IDE"
echo "2. Vá em Settings → Models → API Keys"
echo "3. Verifique:"
echo "   ✅ Toggle 'Use my own Anthropic (Claude) key' está ATIVADO"
echo "   ✅ API key está inserida e mostra 'Verified'"
echo "   ✅ 'Override OpenAI Base URL' está DESATIVADO"
echo ""
echo "4. No chat, verifique que o modelo selecionado é:"
echo "   ✅ claude-3-7-sonnet-20250219 (ou versão mais recente)"
echo ""
echo "5. Se ainda não funcionar:"
echo "   - Reinicie o Cursor completamente"
echo "   - Verifique a API key no console da Anthropic"
echo "   - Teste a API key com curl (veja CONFIGURAR_CLAUDE_API.md)"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Verificar se há processos do Cursor rodando
if pgrep -f "Cursor" > /dev/null; then
    echo -e "${GREEN}✅ Cursor está rodando${NC}"
    echo "   Você pode verificar as configurações agora"
else
    echo -e "${YELLOW}⚠️  Cursor não está rodando${NC}"
    echo "   Abra o Cursor para verificar as configurações"
fi

echo ""
