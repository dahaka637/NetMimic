#pragma once

// -----------------------------------------------------------------------------
// Inclui Winsock2 antes de Windows.h para evitar conflitos
// -----------------------------------------------------------------------------
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

// -----------------------------------------------------------------------------
// Ponteiros para funções originais (serão definidos em hook.cpp)
// -----------------------------------------------------------------------------
typedef int (WSAAPI* tSend)(SOCKET s, const char* buf, int len, int flags);
typedef int (WSAAPI* tRecv)(SOCKET s, char* buf, int len, int flags);

typedef int (WSAAPI* tWSASend)(
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent,
    DWORD dwFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );

typedef int (WSAAPI* tWSARecv)(
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesRecvd,
    LPDWORD lpFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );

// -----------------------------------------------------------------------------
// Namespace principal do hook
// -----------------------------------------------------------------------------
namespace Hook
{
    // Inicializa os hooks (send/recv/WSASend/WSARecv)
    bool Initialize();

    // Remove os hooks e limpa recursos
    void Shutdown();
}
