# ✅ Como Confirmar que Você Está Usando Sua Própria API Key do Claude

## 🔍 Verificação Rápida (2 minutos)

### Passo 1: Abrir Configurações do Cursor
1. Abra o Cursor IDE
2. Pressione `Cmd + ,` (ou vá em **Cursor → Settings**)
3. No campo de busca, digite: `api key`

### Passo 2: Verificar Configuração da API Key
1. Clique em **"Models"** no menu lateral esquerdo
2. Depois clique em **"API Keys"**
3. Procure pela seção **"Anthropic (Claude)"**

### Passo 3: Confirmar que Está Configurado ✅
Você DEVE ver:

```
┌─────────────────────────────────────────┐
│ Anthropic (Claude)                      │
│                                         │
│ ☑ Use my own Anthropic (Claude) key    │ ← DEVE estar MARCADO
│                                         │
│ API Key: [sk-ant-api03-...]            │ ← DEVE mostrar sua key
│ Status: ✓ Verified                      │ ← DEVE mostrar "Verified"
└─────────────────────────────────────────┘
```

### Passo 4: Verificar no Chat
1. Abra um novo chat no Cursor
2. No topo do chat, clique no ícone do modelo (geralmente mostra "Claude" ou o nome do modelo)
3. Você DEVE ver opções como:
   - `claude-3-7-sonnet-20250219` (ou versão mais recente)
   - Se aparecer "Using your API key" ou similar, está correto ✅

## ⚠️ Se NÃO Estiver Configurado

### Opção A: Configurar Agora
1. Em **Settings → Models → API Keys**
2. Encontre **"Anthropic (Claude)"**
3. **ATIVE** o toggle **"Use my own Anthropic (Claude) key"**
4. **COLE** sua API key do Claude
5. Clique em **"Verify"** ou **"Save"**
6. Deve aparecer **"✓ Verified"** ou **"Verified"**

### Opção B: Obter Sua API Key
1. Acesse: https://console.anthropic.com/
2. Faça login
3. Vá em **"API Keys"** no menu
4. Clique em **"Create Key"** (se não tiver uma)
5. Copie a chave (ela começa com `sk-ant-api03-...`)

## 🧪 Teste Rápido para Confirmar

### Teste 1: Verificar Uso da API Key
1. Abra o chat do Cursor
2. Digite uma pergunta simples: "Olá, você está usando minha API key?"
3. Se funcionar sem erro 401, está usando sua key ✅

### Teste 2: Verificar no Console da Anthropic
1. Acesse: https://console.anthropic.com/
2. Vá em **"Usage"** ou **"API Usage"**
3. Faça uma pergunta no chat do Cursor
4. Recarregue a página de Usage
5. Se aparecer uso recente, está usando sua key ✅

## ❌ Problemas Comuns

### Erro 401 (Authentication Fails)
**Causa:** API key não configurada ou inválida

**Solução:**
1. Verifique se o toggle está ATIVADO
2. Verifique se a key está completa (não cortada)
3. Teste a key diretamente:
   ```bash
   curl https://api.anthropic.com/v1/messages \
     -H "x-api-key: SUA_API_KEY_AQUI" \
     -H "anthropic-version: 2023-06-01" \
     -H "content-type: application/json" \
     -d '{"model": "claude-3-7-sonnet-20250219", "max_tokens": 10, "messages": [{"role": "user", "content": "test"}]}'
   ```

### "Override OpenAI Base URL" Ativado
**Causa:** Esta configuração pode quebrar requisições Anthropic

**Solução:**
1. Em **Settings → Models → API Keys**
2. Procure por **"Override OpenAI Base URL"**
3. **DESATIVE** esta opção
4. Reinicie o Cursor

## ✅ Checklist Final

Antes de usar o chat, confirme:

- [ ] Toggle "Use my own Anthropic (Claude) key" está **ATIVADO**
- [ ] API key está inserida e mostra **"Verified"**
- [ ] "Override OpenAI Base URL" está **DESATIVADO**
- [ ] Modelo Claude está selecionado no chat
- [ ] Cursor foi reiniciado após configurar

## 📞 Se Ainda Não Funcionar

1. Feche completamente o Cursor (Cmd + Q)
2. Abra novamente
3. Verifique novamente as configurações
4. Consulte: `CONFIGURAR_CLAUDE_API.md` para mais detalhes

---

**Lembre-se:** O Cursor NÃO armazena a API key em arquivos do projeto. Ela é gerenciada pela interface gráfica do Cursor e armazenada de forma segura no sistema.
