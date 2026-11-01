#include "ThreadReader.h"
#include <winternl.h>

using QuerySystem = NTSTATUS(NTAPI*)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

std::wstring GetState(ULONG state, ULONG waitReason) {
    if (state == 5) return (waitReason == 5 || waitReason == 12) ? L"Suspended" : L"Waiting";
    switch (state) {
    case 0: return L"Initialized"; case 1: return L"Ready";
    case 2: return L"Running"; case 3: return L"Standby";
    case 4: return L"Terminated"; case 6: return L"Transition";
    case 7: return L"DeferredReady"; default: return L"Unknown";
    }
}

bool ReadThreads(DWORD pid, std::vector<ThreadInfo>& out, std::wstring& error) {
    out.clear(); error.clear();
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    auto query = (QuerySystem)GetProcAddress(ntdll, "NtQuerySystemInformation");
    if (!query) return false;

    std::vector<uint8_t> buffer(1024 * 1024);
    ULONG needed = 0;

    while (query(SystemProcessInformation, buffer.data(), (ULONG)buffer.size(), &needed) == (NTSTATUS)0xC0000004) {
        buffer.resize(buffer.size() * 2);
    }

    size_t pos = 0;
    while (pos < buffer.size()) {
        auto* process = (SYSTEM_PROCESS_INFORMATION*)(buffer.data() + pos);

        if ((DWORD)(uintptr_t)process->UniqueProcessId == pid) {
            size_t threadStart = pos + sizeof(SYSTEM_PROCESS_INFORMATION);
            for (ULONG i = 0; i < process->NumberOfThreads; i++) {
                SYSTEM_THREAD_INFORMATION tInfo;
                memcpy(&tInfo, buffer.data() + threadStart + (i * sizeof(tInfo)), sizeof(tInfo));

                ThreadInfo item;
                item.id = (DWORD)(uintptr_t)tInfo.ClientId.UniqueThread;
                item.state = GetState(tInfo.ThreadState, tInfo.WaitReason);
                out.push_back(item);
            }
            return true;
        }

        if (process->NextEntryOffset == 0) break;
        pos += process->NextEntryOffset;
    }

    error = L"Khong lay duoc Threads";
    return false;
}