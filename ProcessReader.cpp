#include "ProcessReader.h"
#include <vector>

bool ReadBasicProcessInfo(HANDLE process, BasicProcessInfo& out, std::wstring& error) {
    out = {};
    error.clear();

    BOOL isWow64 = FALSE;
    if (!IsWow64Process(process, &isWow64)) {
        error = L"Loi IsWow64Process, ma loi: " + std::to_wstring(GetLastError());
        return false;
    }

    out.wow64 = (isWow64 != FALSE);
    out.architecture = out.wow64 ? L"32-bit" : L"64-bit";

    std::vector<wchar_t> pathBuffer(32768);
    DWORD chars = static_cast<DWORD>(pathBuffer.size());

    if (QueryFullProcessImageNameW(process, 0, pathBuffer.data(), &chars)) {
        out.path.assign(pathBuffer.data(), chars);
    }
    else {
        out.path = L"unknown";
        error = L"Loi QueryFullProcessImageNameW, ma loi: " + std::to_wstring(GetLastError());
        return false;
    }

    return true;
}