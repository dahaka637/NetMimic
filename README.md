# NetMimic

🔍 **Socket-Level Traffic Interceptor & Manipulator (Windows)**  
**NetMimic** is a C++ DLL that leverages [MinHook](https://github.com/TsudaKageyu/minhook) to intercept Winsock API calls (`send`, `recv`, `WSASend`, `WSARecv`) inside Windows processes.

It enables developers and researchers to **inspect, log, and modify** network traffic (e.g., HTTP over raw sockets) in real time according to configurable rules.

---

## ✨ Features

- ✅ Hooks Winsock functions: `send`, `recv`, `WSASend`, `WSARecv`.
- ✅ Console output of all intercepted traffic (sent & received).
- ✅ **Mimification engine**:
  - Block or alter packets in-flight.
  - Replace responses with custom payloads.
  - Rule-driven transformations via `mimificado.map`.
- ✅ Automatic `Content-Length` recalculation to avoid protocol errors.
- ✅ Configurable behavior through `config.ini`.
- ✅ Optional persistent logging of all original traffic (`original.log`).

---

## 📂 Project Structure

```
NetMimic/
├── hook.cpp/.h       # Hooking logic (send/recv/WSA* wrappers)
├── mimic.cpp/.h      # Rule processing (mimification engine)
├── config.cpp/.h     # INI configuration management
├── logger.cpp/.h     # Logging system
├── utils.cpp/.h      # Utility functions
├── dllmain.cpp       # DLL entry point
├── config.ini        # Runtime configuration
└── mimificado.map    # Rule definitions for manipulation
```

---

## ⚙️ Installation & Usage

1. **Clone the repository:**
   ```bash
   git clone https://github.com/dahaka637/NetMimic.git
   cd NetMimic
   ```

2. **Open in Visual Studio**
   - Project type: **DLL**
   - Target: **x64** (or x86 depending on the target process)
   - Link with `ws2_32.lib`

3. **Build** → Generates `NetMimic.dll`.

4. **Inject the DLL into a target process**
   - Example tools: [Process Hacker](https://processhacker.sourceforge.io/), [dllinjector](https://github.com/OpenSecurityResearch/dllinjector).
   - On successful injection, a console window will appear showing intercepted traffic.

---

## ⚙️ Configuration (`config.ini`)

Automatically created on first execution:

```ini
[GENERAL]
Intercept=1
LogOriginal=1
```

- **Intercept**: `1` enables packet manipulation; `0` only logs.
- **LogOriginal**: `1` saves all original traffic into `original.log`.

---

## 📝 Rule File (`mimificado.map`)

Defines conditions and transformations for traffic modification.

### Syntax
```ini
[ENVIADO]
keyword1,keyword2
->
[RECEBIDO]
{"status":"success","message":"Fake response","token":"abc123"}
```

### Supported rule types
- **[ENVIADO] → [[ENVIADO]]**: replace outgoing packet.
- **[RECEBIDO] → [[RECEBIDO]]**: replace incoming packet.
- **[ENVIADO] → [RECEBIDO]**: trigger on outgoing pattern → replace next response.
- **[RECEBIDO] → [RECEBIDO]**: transform incoming packets directly.

### Example
```ini
[RECEBIDO]
error,not found
->
[[RECEBIDO]]
{"status":"success","message":"forced ok"}
```
This transforms any response containing `error` or `not found` into a success JSON.

---

## 📊 Example Execution

```
[SEND] (127 bytes)
{"key":"123","product":"autogk","hwid":"abcd"}

[RECV] (125 bytes)
HTTP/1.1 200 OK
Content-Type: application/json

[Mimic] Blocking RECEIVED and replacing with fake response.
[INTERCEPTED → RECV modified]
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 120

{"status":"success","message":"License validated successfully","token":"abc123"}
```

---

## 📌 Notes & Best Practices

- Interception occurs **before TLS decryption**. If the target process uses SSL/TLS, captured data will appear encrypted.
- To analyze TLS traffic:
  - Use a proxy (Burp Suite, Fiddler, mitmproxy), or
  - Enable `SSLKEYLOGFILE` in the target process to export session keys for Wireshark.
- Ideal use cases: debugging, protocol reverse engineering, educational research.

---

## ⚠️ Legal Disclaimer

This project is intended **only for educational and research purposes**.  
Using NetMimic on software or systems without explicit permission may be illegal.  
Always restrict usage to controlled environments or your own applications.

---

## 📜 License

MIT License — Free to use, modify, and distribute with proper attribution.
