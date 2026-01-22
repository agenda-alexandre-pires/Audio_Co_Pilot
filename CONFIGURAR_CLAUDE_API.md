# Configuração da API Key do Claude no Cursor

## Problema
O Cursor está retornando erro 401 (Authentication Fails) ao tentar usar a API key do Claude.

## Solução Passo a Passo

### 1. Verificar Configuração da API Key

1. Abra o Cursor IDE
2. Vá em **Settings** (ou `Cmd + ,` no macOS)
3. Navegue até **Models** → **API Keys**
4. Certifique-se de que:
   - ✅ O toggle **"Use my own Anthropic (Claude) key"** está **ATIVADO**
   - ✅ Sua API key está inserida e verificada (deve mostrar "Verified" ou "✓")
   - ✅ A configuração **"Override OpenAI Base URL"** está **DESATIVADA** (isso pode quebrar requisições Anthropic)

### 2. Verificar o Modelo Selecionado

1. No chat do Cursor, clique no ícone de modelo (geralmente no topo do chat)
2. Certifique-se de usar o nome completo do modelo, por exemplo:
   - ✅ `claude-3-7-sonnet-20250219` (correto)
   - ❌ `claude-3-7` (pode não funcionar)

### 3. Verificar a API Key no Console da Anthropic

1. Acesse: https://console.anthropic.com/
2. Verifique se sua API key está:
   - ✅ Ativa
   - ✅ Com créditos disponíveis
   - ✅ Sem restrições de organização que bloqueiem o uso

### 4. Reiniciar o Cursor

Após fazer as alterações:
1. Feche completamente o Cursor
2. Abra novamente
3. Tente usar o chat novamente

## Troubleshooting Adicional

### Se ainda não funcionar:

1. **Limpar cache do Cursor:**
   ```bash
   rm -rf ~/Library/Application\ Support/Cursor/User/globalStorage/state.vscdb*
   ```

2. **Verificar logs do Cursor:**
   - Abra o Developer Tools (`Cmd + Shift + I`)
   - Vá na aba Console
   - Procure por erros relacionados a "anthropic" ou "api"

3. **Testar a API key diretamente:**
   ```bash
   curl https://api.anthropic.com/v1/messages \
     -H "x-api-key: SUA_API_KEY_AQUI" \
     -H "anthropic-version: 2023-06-01" \
     -H "content-type: application/json" \
     -d '{"model": "claude-3-7-sonnet-20250219", "max_tokens": 10, "messages": [{"role": "user", "content": "test"}]}'
   ```

## Nota Importante

O Cursor **não armazena** a API key em arquivos de configuração do projeto. As configurações são gerenciadas através da interface gráfica do Cursor e armazenadas de forma segura no sistema.

Se você precisa compartilhar a configuração entre máquinas, você precisará configurar manualmente em cada instalação do Cursor.
