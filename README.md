# NetMimic

🔍 **Interceptador e adulterador de tráfego em nível de socket (Windows)**  
O **NetMimic** é uma DLL desenvolvida em C++ que utiliza *hooking* via [MinHook](https://github.com/TsudaKageyu/minhook) para interceptar chamadas de rede (`send`, `recv`, `WSASend`, `WSARecv`) em aplicações Windows.  

Ele permite **inspecionar, registrar e manipular** tráfego HTTP/Socket em tempo real, com base em regras configuradas pelo usuário.

---

## ✨ Funcionalidades

- ✅ Intercepta chamadas `send/recv` e `WSASend/WSARecv` em aplicações.
- ✅ Exibe todo tráfego enviado e recebido no console em tempo real.
- ✅ Suporte a **mimificação**:
  - Bloqueia/envolve pacotes.
  - Substitui mensagens de resposta por conteúdo adulterado.
  - Regras definidas em arquivo `mimificado.map`.
- ✅ Corrige automaticamente cabeçalhos **Content-Length** para evitar falhas.
- ✅ Configuração simples em `config.ini`.
- ✅ Gera logs (`original.log`) de todo tráfego original, se habilitado.

---

## 📂 Estrutura do Projeto

```
NetMimic/
├── hook.cpp/.h       # Implementação dos hooks
├── mimic.cpp/.h      # Sistema de regras de mimificação
├── config.cpp/.h     # Gerenciamento do config.ini
├── logger.cpp/.h     # Sistema de log
├── utils.cpp/.h      # Funções auxiliares
├── dllmain.cpp       # Ponto de entrada da DLL
├── config.ini        # Arquivo de configuração
└── mimificado.map    # Arquivo de regras de adulteração
```

---

## ⚙️ Instalação e Uso

1. **Clone o repositório:**
   ```bash
   git clone https://github.com/dahaka637/NetMimic.git
   cd NetMimic
   ```

2. **Abra o projeto no Visual Studio**  
   - Certifique-se de compilar como **DLL**.
   - Plataforma: **x64** (ou x86 conforme o alvo).
   - Link com `ws2_32.lib`.

3. **Compile a DLL** → será gerada `NetMimic.dll`.

4. **Carregue a DLL no processo alvo**  
   - Use um injetor de DLL (ex: [Process Hacker](https://processhacker.sourceforge.io/), [DLL Injector](https://github.com/OpenSecurityResearch/dllinjector)).
   - Assim que a DLL for carregada, um **console** será aberto mostrando as mensagens interceptadas.

---

## ⚙️ Configuração (`config.ini`)

Arquivo gerado automaticamente no primeiro uso:

```ini
[GERAL]
Interceptar=1
LogOriginal=1
```

- **Interceptar**: `1` ativa a adulteração de tráfego, `0` apenas exibe/loga.  
- **LogOriginal**: `1` grava todo tráfego interceptado em `original.log`.

---

## 📝 Regras de Mimificação (`mimificado.map`)

As regras definem **quando e como adulterar mensagens**.  
Formato básico:

```ini
[ENVIADO]
keyword1,keyword2
->
[RECEBIDO]
{"status":"success","message":"Fake response","token":"abc123"}
```

### Tipos suportados:
- **[ENVIADO] -> [[ENVIADO]]**: substitui o pacote enviado.  
- **[RECEBIDO] -> [[RECEBIDO]]**: substitui imediatamente o recebido.  
- **[ENVIADO] -> [RECEBIDO]**: aguarda envio com palavras-chave → substitui próximo recebido.  
- **[RECEBIDO] -> [RECEBIDO]**: substitui recebido diretamente.

Exemplo prático:

```ini
[RECEBIDO]
error,not found
->
[[RECEBIDO]]
{"status":"success","message":"forced ok"}
```

Esse exemplo transforma toda resposta que contenha `"error"` ou `"not found"` em um JSON de sucesso.

---

## 📊 Exemplo de Execução

```
[SEND] (127 bytes)
{"key":"123","product":"autogk","hwid":"abcd"}

[RECV] (125 bytes)
HTTP/1.1 200 OK
Content-Type: application/json

[Mimic] Bloqueando RECEBIDO e substituindo pelo fake.
[INTERCEPTADO → RECV adulterado]
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 120

{"status":"success","message":"License validated successfully","token":"abc123"}
```

---

## ⚠️ Aviso Legal

Este projeto foi desenvolvido **apenas para fins educacionais e de pesquisa**.  
O uso em sistemas de terceiros sem autorização pode ser ilegal.  
Use o **NetMimic** com responsabilidade.

---

## 📜 Licença

MIT — Livre para uso, modificação e distribuição, mantendo os devidos créditos.
