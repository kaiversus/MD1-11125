#include "ModuleReader.h"
#include <TlHelp32.h>

bool ReadModules(DWORD pid, std::vector<ModuleInfo>& out, std::wstring& error) {
    out.clear(); error.clear();

    HANDLE snapshot = INVALID_HANDLE_VALUE;
    for (int i = 0; i < 4; i++) {
        snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snapshot != INVALID_HANDLE_VALUE || GetLastError() != ERROR_BAD_LENGTH) break;
    }

    if (snapshot == INVALID_HANDLE_VALUE) {
        error = L"Loi chup anh DLL"; return false;
    }

    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (Module32FirstW(snapshot, &entry)) {
        do {
            std::wstring name = entry.szModule;
            if (name.length() >= 4 && _wcsicmp(name.substr(name.length() - 4).c_str(), L".dll") == 0) {
                ModuleInfo m;
                m.name = entry.szModule;
                m.path = entry.szExePath;
                out.push_back(m);
            }
        } while (Module32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return true;
}