#pragma once
#include <windows.h>
#include <string>

struct BasicProcessInfo {
    std::wstring architecture;
    std::wstring path;
    bool wow64 = false;
};

bool ReadBasicProcessInfo(HANDLE process, BasicProcessInfo& out, std::wstring& error);