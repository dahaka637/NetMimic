#include "utils.h"
#include <windows.h>
#include <sstream>

// Retorna o diretório da DLL injetada
std::string Utils::GetDllPath()
{
    char path[MAX_PATH];
    HMODULE hModule = nullptr;

    // Pega handle do próprio módulo (nossa DLL)
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&GetDllPath, &hModule))
    {
        GetModuleFileNameA(hModule, path, MAX_PATH);
        std::string fullPath(path);

        // Remove o nome da DLL, ficando só o diretório
        size_t pos = fullPath.find_last_of("\\/");
        if (pos != std::string::npos)
            return fullPath.substr(0, pos);
        else
            return fullPath;
    }

    return ".";
}

// Formata JSON simples para leitura
std::string Utils::PrettyJson(const std::string& s)
{
    if (s.find('{') == std::string::npos && s.find('[') == std::string::npos)
        return s; // não parece JSON → retorna cru

    std::ostringstream out;
    int indent = 0;
    for (char c : s)
    {
        switch (c)
        {
        case '{': case '[':
            out << c << "\n" << std::string(++indent * 2, ' ');
            break;
        case '}': case ']':
            out << "\n" << std::string(--indent * 2, ' ') << c;
            break;
        case ',':
            out << c << "\n" << std::string(indent * 2, ' ');
            break;
        default:
            out << c;
        }
    }
    return out.str();
}
