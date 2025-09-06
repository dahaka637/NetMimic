#include "logger.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <iomanip>

static std::string logPath;
static std::ofstream logFile;

bool Logger::Initialize(const ConfigData& cfg)
{
    try {
        // Pega diret�rio da DLL
        std::string basePath = Utils::GetDllPath();

        // Arquivo original.log
        logPath = basePath + "\\original.log";

        if (!std::filesystem::exists(logPath))
        {
            std::ofstream createFile(logPath);
            createFile << "=============================================\n";
            createFile << "===   Log de tr�fego interceptado (in�cio) ===\n";
            createFile << "=============================================\n\n";
            createFile.close();
        }

        // Abre em append
        logFile.open(logPath, std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "[Logger] Falha ao abrir original.log" << std::endl;
            return false;
        }

        // Tamb�m cria mimificado.map se n�o existir
        std::string mimicPath = basePath + "\\mimificado.map";
        if (!std::filesystem::exists(mimicPath))
        {
            std::ofstream createMimic(mimicPath);
            createMimic << "# Arquivo de mimifica��o (identifica��o)\n";
            createMimic << "# Exemplo de regra:\n";
            createMimic << "[ENVIADO]\n";
            createMimic << "key,product,hwid,autogk\n";
            createMimic << "->\n";
            createMimic << "[RECEBIDO]\n";
            createMimic << "{\"status\":\"success\",\"message\":\"License validated successfully\",\"token\":\"abc123\",\"expires_at\":\"2099-12-31 23:59:59\",\"license_remaining_time\":\"9999 days\"}\n";
            createMimic.close();
        }

        std::cout << "[Logger] original.log e mimificado.map inicializados." << std::endl;
        return true;

    }
    catch (const std::exception& ex) {
        std::cerr << "[Logger] Erro: " << ex.what() << std::endl;
        return false;
    }
}

void Logger::Shutdown()
{
    if (logFile.is_open())
        logFile.close();
}

void Logger::Log(const std::string& tipo, const std::string& data)
{
    if (!logFile.is_open())
        return;

    // Timestamp bonitinho
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &now_c);

    // Cabe�alho formatado
    logFile << "---------------------------------------------\n";
    logFile << tipo << " @ "
        << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << "\n";
    logFile << "---------------------------------------------\n";

    // Conte�do
    logFile << data << "\n\n";
    logFile.flush();
}
