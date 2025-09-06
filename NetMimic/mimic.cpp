#include "mimic.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <sstream>
#include <algorithm>

static ConfigData g_cfg;

// Estrutura interna de regra
struct MimicRule {
    std::string triggerTipo;           // [ENVIADO] ou [RECEBIDO]
    std::vector<std::string> keywords; // palavras-chave obrigatórias
    std::string replaceTipo;           // [RECEBIDO], [ENVIADO], [[ENVIADO]], [[RECEBIDO]]
    std::string replacement;           // conteúdo substituto
};

static std::vector<MimicRule> mimicRules;
static std::string pendingReplacement; // usado para interceptação encadeada

// --- Helpers ---
static std::string ToLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return r;
}

static std::vector<std::string> SplitKeywords(const std::string& line) {
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item.erase(remove_if(item.begin(), item.end(), ::isspace), item.end());
        if (!item.empty())
            result.push_back(ToLower(item));
    }
    return result;
}

// --- Inicializa e carrega mimificado.map ---
bool Mimic::Initialize(const ConfigData& cfg)
{
    g_cfg = cfg;
    mimicRules.clear();
    pendingReplacement.clear();

    try {
        std::string basePath = Utils::GetDllPath();
        std::string mimicPath = basePath + "\\mimificado.map";

        // Se não existir, cria exemplo inicial
        if (!std::filesystem::exists(mimicPath)) {
            std::ofstream out(mimicPath);
            out << "# Arquivo de mimificação (multi-regra)\n";
            out << "# Suporta interceptação imediata ([[...]]), encadeada e direta.\n\n";

            // Exemplo 1: interceptar ENVIADO direto
            out << "[ENVIADO]\n";
            out << "key,hwid\n";
            out << "->\n";
            out << "[[ENVIADO]]\n";
            out << "{\"key\":\"fake\",\"hwid\":\"xxxx\"}\n\n";

            // Exemplo 2: interceptar ENVIADO mas trocar próximo RECEBIDO
            out << "[ENVIADO]\n";
            out << "product,autogk\n";
            out << "->\n";
            out << "[RECEBIDO]\n";
            out << "{\"status\":\"success\",\"token\":\"abc123\"}\n\n";

            // Exemplo 3: interceptar RECEBIDO direto
            out << "[RECEBIDO]\n";
            out << "error,not found\n";
            out << "->\n";
            out << "[[RECEBIDO]]\n";
            out << "{\"status\":\"success\",\"message\":\"forced ok\"}\n";
            out.close();
        }

        // Carrega regras do arquivo
        std::ifstream in(mimicPath);
        if (!in.is_open()) return false;

        std::string line;
        MimicRule current;
        enum class State { None, Trigger, Keywords, ReplacementTipo, Replacement } state = State::None;

        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#' || line[0] == ';')
                continue;

            if (line == "->") {
                state = State::ReplacementTipo;
                continue;
            }

            if (line == "[ENVIADO]" || line == "[RECEBIDO]") {
                if (state == State::None) {
                    current = MimicRule{};
                    current.triggerTipo = line;
                    state = State::Keywords;
                }
                else if (state == State::ReplacementTipo) {
                    current.replaceTipo = line;
                    state = State::Replacement;
                }
                continue;
            }

            if (line == "[[ENVIADO]]" || line == "[[RECEBIDO]]") {
                current.replaceTipo = line;
                state = State::Replacement;
                continue;
            }

            if (state == State::Keywords) {
                current.keywords = SplitKeywords(line);
            }
            else if (state == State::Replacement) {
                current.replacement = line;
                mimicRules.push_back(current);
                state = State::None;
            }
        }
        in.close();

        std::cout << "[Mimic] Carregado " << mimicRules.size() << " regras." << std::endl;
        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "[Mimic] Erro: " << ex.what() << std::endl;
        return false;
    }
}

void Mimic::Shutdown()
{
    mimicRules.clear();
    pendingReplacement.clear();
}

// --- Aplica mimificação ---
std::string Mimic::Process(const std::string& tipo, const std::string& data)
{
    if (g_cfg.interceptar == 0)
        return data;

    std::string dataLower = ToLower(data);

    // Caso haja interceptação pendente (ENVIADO -> RECEBIDO)
    if (!pendingReplacement.empty() && tipo == "[RECEBIDO]") {
        std::string tmp = pendingReplacement;
        pendingReplacement.clear();
        std::cout << "[Mimic] Resposta interceptada (encadeada) substituída." << std::endl;
        std::cout << "[INTERCEPTADO → SUBSTITUÍDO no RECEBIDO]" << std::endl;
        return tmp;
    }

    for (auto& rule : mimicRules) {
        if (rule.triggerTipo != tipo)
            continue;

        // Checa palavras-chave
        bool allMatch = true;
        for (auto& kw : rule.keywords) {
            if (dataLower.find(kw) == std::string::npos) {
                allMatch = false;
                break;
            }
        }
        if (!allMatch) continue;

        // --- Interceptação imediata no ENVIADO ---
        if (rule.replaceTipo == "[[ENVIADO]]" && tipo == "[ENVIADO]") {
            std::cout << "[Mimic] Bloqueando ENVIADO e substituindo pelo fake." << std::endl;
            std::cout << "[INTERCEPTADO → SUBSTITUÍDO no ENVIADO]" << std::endl;
            return rule.replacement;
        }

        // --- Interceptação imediata no RECEBIDO ---
        if (rule.replaceTipo == "[[RECEBIDO]]" && tipo == "[RECEBIDO]") {
            std::cout << "[Mimic] Bloqueando RECEBIDO e substituindo pelo fake." << std::endl;
            std::cout << "[INTERCEPTADO → SUBSTITUÍDO no RECEBIDO]" << std::endl;
            return rule.replacement;
        }

        // --- Encadeado ENVIADO -> RECEBIDO ---
        if (tipo == "[ENVIADO]" && rule.replaceTipo == "[RECEBIDO]") {
            pendingReplacement = rule.replacement;
            std::cout << "[Mimic] Marcado para interceptar próximo RECEBIDO." << std::endl;
            return data; // segue envio original
        }

        // --- Substituição direta RECEBIDO -> RECEBIDO ---
        if (rule.replaceTipo == "[RECEBIDO]" && tipo == "[RECEBIDO]") {
            std::cout << "[Mimic] Substituindo RECEBIDO imediatamente." << std::endl;
            std::cout << "[INTERCEPTADO → SUBSTITUÍDO no RECEBIDO]" << std::endl;
            return rule.replacement;
        }
    }

    return data; // sem alteração
}

// --- Controle de fluxo: ENVIADO -> RECEBIDO ---
bool Mimic::TriggeredSendRequiresRecv(const std::string& data)
{
    if (g_cfg.interceptar == 0) return false;

    std::string dataLower = ToLower(data);
    for (auto& rule : mimicRules) {
        if (rule.triggerTipo != "[ENVIADO]") continue;
        if (rule.replaceTipo != "[RECEBIDO]") continue;

        bool allMatch = true;
        for (auto& kw : rule.keywords) {
            if (dataLower.find(kw) == std::string::npos) {
                allMatch = false;
                break;
            }
        }
        if (allMatch) return true;
    }
    return false;
}

// --- Utilidades ---
const std::vector<Mimic::Rule>& Mimic::GetRules()
{
    static std::vector<Mimic::Rule> exported;
    exported.clear();
    for (auto& r : mimicRules) {
        exported.push_back({ r.triggerTipo, r.keywords.empty() ? "" : r.keywords[0], r.replaceTipo, r.replacement });
    }
    return exported;
}

bool Mimic::Reload(const ConfigData& cfg)
{
    Shutdown();
    return Initialize(cfg);
}
