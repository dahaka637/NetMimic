#pragma once
#include <string>
#include "config.h"

namespace Logger
{
    bool Initialize(const ConfigData& cfg); // cria arquivos se n�o existirem
    void Shutdown();                        // fecha se necess�rio
    void Log(const std::string& tipo, const std::string& data); // escreve no original.log
}
