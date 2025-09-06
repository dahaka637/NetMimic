#pragma once
#include <string>

struct ConfigData {
    int interceptar = 1;   // se deve interceptar (1 = sim, 0 = n�o)
    int logOriginal = 1;   // se deve logar dados originais
};

namespace Config {
    extern ConfigData Current;

    bool LoadOrCreate(ConfigData& cfg);
}
