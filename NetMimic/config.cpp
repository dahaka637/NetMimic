#include "config.h"
#include "utils.h"

#include <fstream>
#include <iostream>
#include <filesystem>

ConfigData Config::Current;

static void WriteDefaultConfig(const std::string& path)
{
    std::ofstream out(path);
    out << "[GERAL]\n";
    out << "Interceptar=1\n";   // por padrão, já intercepta
    out << "LogOriginal=1\n";   // por padrão, já loga
    out.close();
}

bool Config::LoadOrCreate(ConfigData& cfg)
{
    try {
        std::string basePath = Utils::GetDllPath();
        std::string configPath = basePath + "\\config.ini";

        if (!std::filesystem::exists(configPath)) {
            WriteDefaultConfig(configPath);
        }

        std::ifstream in(configPath);
        if (!in.is_open()) {
            std::cerr << "[Config] Falha ao abrir config.ini" << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == ';' || line[0] == '#')
                continue;

            auto pos = line.find('=');
            if (pos == std::string::npos) continue;

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            if (key == "Interceptar") cfg.interceptar = std::stoi(value);
            else if (key == "LogOriginal") cfg.logOriginal = std::stoi(value);
        }

        in.close();
        Config::Current = cfg;

        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "[Config] Erro ao processar config.ini: " << ex.what() << std::endl;
        return false;
    }
}
