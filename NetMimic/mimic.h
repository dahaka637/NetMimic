#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "config.h"

namespace Mimic
{
    // ============================================================
    // Estrutura de uma regra de mimificação
    // ============================================================
    struct Rule {
        std::string fromType;   // [ENVIADO] ou [RECEBIDO]
        std::string pattern;    // string a procurar (ex: "hwid")
        std::string toType;     // destino da regra ([ENVIADO] ou [RECEBIDO])
        std::string replacement; // texto a substituir/adulterar
    };

    // ============================================================
    // Inicialização / Finalização
    // ============================================================

    // Carrega as regras do arquivo mimificado.map (ou config)
    bool Initialize(const ConfigData& cfg);

    // Libera memória, regras carregadas
    void Shutdown();

    // ============================================================
    // Processamento de dados
    // ============================================================

    // Processa o dado (enviado ou recebido) aplicando regras
    // tipo → "[ENVIADO]" ou "[RECEBIDO]"
    std::string Process(const std::string& tipo, const std::string& data);

    // ============================================================
    // Controle de fluxo (ex: [ENVIADO] -> [RECEBIDO])
    // ============================================================

    // Verifica se um dado enviado ativa regra que exige adulteração
    // no próximo recebimento (flow control)
    bool TriggeredSendRequiresRecv(const std::string& data);

    // ============================================================
    // Utilidades (opcional)
    // ============================================================

    // Retorna todas as regras carregadas (debug/inspeção)
    const std::vector<Rule>& GetRules();

    // Força recarregar as regras do mapa
    bool Reload(const ConfigData& cfg);
}
