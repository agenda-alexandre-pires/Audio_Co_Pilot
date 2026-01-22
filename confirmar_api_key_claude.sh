#!/bin/bash

# Script para confirmar que a API key do Claude está configurada

echo "🔍 Verificando configuração da API Key do Claude no Cursor..."
echo ""

# Cores
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

# Caminho do banco de dados do Cursor
DB_PATH="$HOME/Library/Application Support/Cursor/User/globalStorage/state.vscdb"

if [ ! -f "$DB_PATH" ]; then
    echo -e "${RED}❌ Banco de dados do Cursor não encontrado${NC}"
    exit 1
fi

# Verificar se sqlite3 está disponível
if ! command -v sqlite3 &> /dev/null; then
    echo -e "${YELLOW}⚠️  sqlite3 não encontrado. Instalando verificação alternativa...${NC}"
    # Tentar verificar de outra forma
    if grep -q "claudeKey" "$HOME/Library/Application Support/Cursor/User/globalStorage/storage.json" 2>/dev/null; then
        echo -e "${GREEN}✅ API key do Claude encontrada no storage.json${NC}"
    else
        echo -e "${YELLOW}⚠️  Não foi possível verificar automaticamente${NC}"
    fi
    exit 0
fi

# Buscar API key do Claude no banco de dados
API_KEY=$(sqlite3 "$DB_PATH" "SELECT value FROM ItemTable WHERE key = 'cursorAuth/claudeKey';" 2>/dev/null)

if [ -z "$API_KEY" ]; then
    echo -e "${RED}❌ API Key do Claude NÃO encontrada no Cursor${NC}"
    echo ""
    echo "📋 Para configurar:"
    echo "   1. Abra o Cursor IDE"
    echo "   2. Vá em Settings → Models → API Keys"
    echo "   3. Ative 'Use my own Anthropic (Claude) key'"
    echo "   4. Insira sua API key"
    exit 1
fi

# API key encontrada
echo -e "${GREEN}✅ API KEY DO CLAUDE ENCONTRADA E CONFIGURADA!${NC}"
echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "${GREEN}📌 Status:${NC}"
echo "   API Key: ${API_KEY:0:20}...${API_KEY: -10}"
echo "   (Key truncada por segurança)"
echo ""

# Verificar se há aviso de configuração
WARNING=$(sqlite3 "$DB_PATH" "SELECT value FROM ItemTable WHERE key = 'cursor/settingsDismissedClaudeKeyWarning';" 2>/dev/null)

if [ "$WARNING" = "false" ]; then
    echo -e "${YELLOW}⚠️  Aviso de configuração ainda não foi dispensado${NC}"
    echo "   Isso pode indicar que a configuração precisa ser verificada na interface"
fi

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "${GREEN}✅ CONFIRMAÇÃO:${NC}"
echo ""
echo "   Sua API key do Claude ESTÁ configurada no Cursor!"
echo ""
echo "📋 Para garantir que está sendo usada:"
echo ""
echo "   1. Abra o Cursor IDE"
echo "   2. Vá em Settings → Models → API Keys"
echo "   3. Verifique que o toggle 'Use my own Anthropic (Claude) key' está ATIVADO"
echo "   4. Verifique que mostra 'Verified' ou '✓'"
echo "   5. No chat, selecione um modelo Claude"
echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Verificar se o Cursor está rodando
if pgrep -f "Cursor" > /dev/null; then
    echo -e "${GREEN}✅ Cursor está rodando${NC}"
    echo "   Você pode verificar as configurações agora mesmo!"
else
    echo -e "${YELLOW}⚠️  Cursor não está rodando${NC}"
    echo "   Abra o Cursor para verificar as configurações"
fi

echo ""
