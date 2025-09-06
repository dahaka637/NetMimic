#include "hook.h"
#include "logger.h"
#include "mimic.h"
#include "config.h"
#include "utils.h"

#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <cctype>

#include "MinHook/include/MinHook.h"

#pragma comment(lib, "ws2_32.lib")

// -----------------------------------------------------------------------------
// Ponteiros originais
// -----------------------------------------------------------------------------
typedef int (WSAAPI* tSend)(SOCKET s, const char* buf, int len, int flags);
typedef int (WSAAPI* tRecv)(SOCKET s, char* buf, int len, int flags);
typedef int (WSAAPI* tWSASend)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent, DWORD dwFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
typedef int (WSAAPI* tWSARecv)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

static tSend     oSend = nullptr;
static tRecv     oRecv = nullptr;
static tWSASend  oWSASend = nullptr;
static tWSARecv  oWSARecv = nullptr;

// -----------------------------------------------------------------------------
// Helper
// -----------------------------------------------------------------------------
static void SetColor(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

// Remove TODAS as linhas Content-Length do header
static void RemoveAllContentLength(std::string& headers) {
    size_t pos = 0;
    while ((pos = headers.find("Content-Length:")) != std::string::npos ||
        (pos = headers.find("content-length:")) != std::string::npos) {
        size_t endLine = headers.find("\r\n", pos);
        if (endLine == std::string::npos) {
            headers.erase(pos);
            break;
        }
        else {
            headers.erase(pos, endLine - pos + 2);
        }
    }
}

// Substitui o body e corrige Content-Length
static std::string PatchHttpResponse(const std::string& original, const std::string& newBody) {
    size_t headerEnd = original.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        // Não parece HTTP → devolve apenas o adulterado
        return newBody;
    }

    std::string headers = original.substr(0, headerEnd);

    // Remove duplicações
    RemoveAllContentLength(headers);

    // Insere o novo Content-Length
    headers += "\r\nContent-Length: " + std::to_string(newBody.size());

    return headers + "\r\n\r\n" + newBody;
}

// -----------------------------------------------------------------------------
// Hook em send
// -----------------------------------------------------------------------------
int WSAAPI HookedSend(SOCKET s, const char* buf, int len, int flags) {
    std::string data(buf, len);

    if (Config::Current.logOriginal) {
        SetColor(11);
        std::cout << "\n[SEND] (" << len << " bytes)\n" << data << std::endl;
        SetColor(7);
        Logger::Log("[SEND]", data);
    }

    // Se não for interceptar → retorna envio original
    if (!Config::Current.interceptar) {
        return oSend(s, buf, len, flags);
    }

    // Processa com Mimic (para ENVIADO normalmente não alteramos,
    // mas deixamos flexível se regras existirem)
    std::string altered = Mimic::Process("[ENVIADO]", data);

    if (!altered.empty() && altered != data) {
        SetColor(10);
        std::cout << "[INTERCEPTADO → SEND adulterado]\n" << altered << std::endl;
        SetColor(7);
        return oSend(s, altered.c_str(), (int)altered.size(), flags);
    }

    return oSend(s, buf, len, flags);
}

// -----------------------------------------------------------------------------
// Hook em recv
// -----------------------------------------------------------------------------
int WSAAPI HookedRecv(SOCKET s, char* buf, int len, int flags) {
    int ret = oRecv(s, buf, len, flags);
    if (ret > 0) {
        std::string data(buf, ret);

        if (Config::Current.logOriginal) {
            SetColor(11);
            std::cout << "\n[RECV] (" << ret << " bytes)\n" << data << std::endl;
            SetColor(7);
            Logger::Log("[RECV]", data);
        }

        if (!Config::Current.interceptar) {
            return ret; // não altera nada
        }

        std::string newBody = Mimic::Process("[RECEBIDO]", data);

        if (!newBody.empty() && newBody != data) {
            std::string patched = PatchHttpResponse(data, newBody);

            size_t fsize = std::min<size_t>(patched.size(), (size_t)len);
            memcpy(buf, patched.data(), fsize);
            ret = (int)fsize;

            SetColor(12);
            std::cout << "[INTERCEPTADO → RECV adulterado]\n" << patched << std::endl;
            SetColor(7);
        }
    }
    return ret;
}


// -----------------------------------------------------------------------------
// Hook em WSASend
// -----------------------------------------------------------------------------
int WSAAPI HookedWSASend(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent, DWORD dwFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {

    if (!lpBuffers || dwBufferCount == 0) return oWSASend(s, lpBuffers, dwBufferCount,
        lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);

    std::string data;
    for (DWORD i = 0; i < dwBufferCount; i++) {
        data.append(lpBuffers[i].buf, lpBuffers[i].len);
    }

    if (Config::Current.logOriginal) {
        SetColor(11);
        std::cout << "\n[WSASend] (" << data.size() << " bytes)\n" << data << std::endl;
        SetColor(7);
        Logger::Log("[WSASend]", data);
    }

    if (!Config::Current.interceptar) {
        return oWSASend(s, lpBuffers, dwBufferCount,
            lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
    }

    std::string altered = Mimic::Process("[ENVIADO]", data);
    if (!altered.empty() && altered != data) {
        SetColor(10);
        std::cout << "[INTERCEPTADO → WSASend adulterado]\n" << altered << std::endl;
        SetColor(7);

        lpBuffers[0].buf = const_cast<char*>(altered.data());
        lpBuffers[0].len = (ULONG)altered.size();
        dwBufferCount = 1;
    }

    return oWSASend(s, lpBuffers, dwBufferCount,
        lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
}

// -----------------------------------------------------------------------------
// Hook em WSARecv
// -----------------------------------------------------------------------------
int WSAAPI HookedWSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {

    int ret = oWSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd,
        lpFlags, lpOverlapped, lpCompletionRoutine);

    if (ret == 0 && lpNumberOfBytesRecvd && *lpNumberOfBytesRecvd > 0) {
        std::string data(lpBuffers[0].buf, *lpNumberOfBytesRecvd);

        if (Config::Current.logOriginal) {
            SetColor(11);
            std::cout << "\n[WSARecv] (" << *lpNumberOfBytesRecvd << " bytes)\n" << data << std::endl;
            SetColor(7);
            Logger::Log("[WSARecv]", data);
        }


        if (Config::Current.interceptar) {
            std::string newBody = Mimic::Process("[RECEBIDO]", data);

            if (!newBody.empty() && newBody != data) {
                std::string patched = PatchHttpResponse(data, newBody);

                size_t fsize = std::min<size_t>(patched.size(), (size_t)lpBuffers[0].len);
                memcpy(lpBuffers[0].buf, patched.data(), fsize);
                *lpNumberOfBytesRecvd = (DWORD)fsize;

                SetColor(12);
                std::cout << "[INTERCEPTADO → WSARecv adulterado]\n" << patched << std::endl;
                SetColor(7);
            }
        }
    }
    return ret;
}

// -----------------------------------------------------------------------------
// Inicialização / desligamento
// -----------------------------------------------------------------------------
bool Hook::Initialize() {
    if (MH_Initialize() != MH_OK) return false;
    if (MH_CreateHookApi(L"ws2_32", "send", &HookedSend, (LPVOID*)&oSend) != MH_OK) return false;
    if (MH_CreateHookApi(L"ws2_32", "recv", &HookedRecv, (LPVOID*)&oRecv) != MH_OK) return false;
    if (MH_CreateHookApi(L"ws2_32", "WSASend", &HookedWSASend, (LPVOID*)&oWSASend) != MH_OK) return false;
    if (MH_CreateHookApi(L"ws2_32", "WSARecv", &HookedWSARecv, (LPVOID*)&oWSARecv) != MH_OK) return false;
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) return false;

    std::cout << "[Hook] send/recv/WSASend/WSARecv hookados com sucesso!\n";
    return true;
}

void Hook::Shutdown() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    std::cout << "[Hook] Hooks removidos.\n";
}
