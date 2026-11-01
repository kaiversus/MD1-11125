#pragma once
#include <windows.h>
#include <string>
#include <vector>

struct ProcessEntry {
    std::wstring name;
    DWORD pid = 0;
    DWORD ppid = 0;
};

struct ThreadInfo {
    DWORD id = 0;
    std::wstring state;
};

struct ModuleInfo {
    std::wstring name;
    std::wstring path;
};

struct ImportInfo {
    std::string dll;
    std::vector<std::string> functions;
};

struct ProcessReport {
    ProcessEntry process;
    std::wstring architecture = L"unavailable";
    std::wstring path = L"unavailable";
    std::wstring commandLine = L"unavailable";
    std::vector<ThreadInfo> threads;
    std::vector<ModuleInfo> modules;
    std::vector<ImportInfo> imports;
};