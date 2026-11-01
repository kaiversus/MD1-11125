#include "PEBReader.h"
#include <winternl.h>
#include <cstdint>

struct Unicode32 { USHORT length; USHORT capacity; uint32_t buffer; };
struct Peb32 { uint8_t reserved[0x10]; uint32_t parameters; };
struct Parameters32 { uint8_t reserved[0x40]; Unicode32 command; };
using QueryFn = NTSTATUS(NTAPI*)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

bool ReadPebCommandLine(HANDLE process, bool wow64, std::wstring& commandLine, std::wstring& error) {
    commandLine.clear();
    error.clear();
    SIZE_T bytesRead = 0;

    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    auto query = (QueryFn)GetProcAddress(ntdll, "NtQueryInformationProcess");
    if (!query) { error = L"Khong the tai NtQueryInformationProcess"; return false; }

    if (wow64) {
        ULONG_PTR peb32Address = 0;
        if (query(process, ProcessWow64Information, &peb32Address, sizeof(peb32Address), nullptr) < 0 || peb32Address == 0) {
            error = L"Loi ProcessWow64Information"; return false;
        }

        Peb32 peb32{};
        ReadProcessMemory(process, (LPCVOID)peb32Address, &peb32, sizeof(peb32), &bytesRead);

        Parameters32 params32{};
        ReadProcessMemory(process, (LPCVOID)(uintptr_t)peb32.parameters, &params32, sizeof(params32), &bytesRead);

        if (params32.command.length > 0 && params32.command.buffer != 0) {
            commandLine.assign(params32.command.length / sizeof(wchar_t), L'\0');
            ReadProcessMemory(process, (LPCVOID)(uintptr_t)params32.command.buffer, commandLine.data(), params32.command.length, &bytesRead);
        }
    }
    else {
        PROCESS_BASIC_INFORMATION basic{};
        if (query(process, ProcessBasicInformation, &basic, sizeof(basic), nullptr) < 0) {
            error = L"Loi ProcessBasicInformation"; return false;
        }

        PEB peb{};
        ReadProcessMemory(process, basic.PebBaseAddress, &peb, sizeof(peb), &bytesRead);

        RTL_USER_PROCESS_PARAMETERS params{};
        ReadProcessMemory(process, peb.ProcessParameters, &params, sizeof(params), &bytesRead);

        if (params.CommandLine.Length > 0 && params.CommandLine.Buffer != nullptr) {
            commandLine.assign(params.CommandLine.Length / sizeof(wchar_t), L'\0');
            ReadProcessMemory(process, params.CommandLine.Buffer, commandLine.data(), params.CommandLine.Length, &bytesRead);
        }
    }
    return true;
}