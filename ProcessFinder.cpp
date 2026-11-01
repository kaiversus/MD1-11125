#include "ProcessFinder.h"
#include <TlHelp32.h>

SearchResult Find(const std::wstring* name, DWORD pid) {
    SearchResult result;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        result.error = GetLastError();
        return result;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry)) {
        do {
            bool match = false;
            if (name != nullptr) {
                match = (CompareStringOrdinal(entry.szExeFile, -1, name->c_str(), -1, TRUE) == CSTR_EQUAL);
            }
            else {
                match = (entry.th32ProcessID == pid);
            }

            if (match) {
                ProcessEntry process;
                process.name = entry.szExeFile;
                process.pid = entry.th32ProcessID;
                process.ppid = entry.th32ParentProcessID;
                result.processes.push_back(process);
            }
        } while (Process32NextW(snapshot, &entry));
    }

    result.error = GetLastError() == ERROR_NO_MORE_FILES ? ERROR_SUCCESS : GetLastError();
    CloseHandle(snapshot);
    return result;
}

SearchResult FindByName(const std::wstring& name) {
    if (name.empty()) return { {}, ERROR_INVALID_PARAMETER };
    return Find(&name, 0);
}

SearchResult FindByPid(DWORD pid) {
    if (pid == 0) return { {}, ERROR_INVALID_PARAMETER };
    return Find(nullptr, pid);
}