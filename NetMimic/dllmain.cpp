#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>

#include "config.h"
#include "logger.h"
#include "hook.h"
#include "mimic.h"
#include "utils.h"

DWORD WINAPI MainThread(LPVOID lpReserved)
{
    // 1. Abre console para debug/saída
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);

    SetConsoleTitleA("Interceptador DLL");
    std::cout << "[DLL] Interceptador iniciado no PID=" << GetCurrentProcessId() << std::endl;

    // 2. Carrega ou cria configuração
    ConfigData config;
    if (!Config::LoadOrCreate(config)) {
        std::cerr << "[DLL] Falha ao carregar/criar config.ini" << std::endl;
        return 1;
    }

    std::cout << "[DLL] Configuração carregada:" << std::endl;
    std::cout << "  Interceptar=" << config.interceptar << std::endl;
    std::cout << "  LogOriginal=" << config.logOriginal << std::endl;

    // 3. Inicializa logger (gera original.log se necessário)
    if (!Logger::Initialize(config)) {
        std::cerr << "[DLL] Falha ao inicializar logger!" << std::endl;
        return 1;
    }

    // 4. Inicializa mecanismo de mimificação (gera mimificado.map se necessário)
    if (!Mimic::Initialize(config)) {
        std::cerr << "[DLL] Falha ao inicializar Mimic!" << std::endl;
        return 1;
    }

    // 5. Instala hooks (send/recv/WSA)
    if (!Hook::Initialize()) {
        std::cerr << "[DLL] Falha ao inicializar hooks!" << std::endl;
        return 1;
    }

    std::cout << "[DLL] Hooks em send/recv ativos!" << std::endl;
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        Hook::Shutdown();
        Mimic::Shutdown();
        Logger::Shutdown();
        break;
    }
    return TRUE;
}
