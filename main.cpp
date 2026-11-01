#include "ProcessFinder.h"
#include "ProcessReader.h"
#include "PEBReader.h"
#include "ThreadReader.h"
#include "ModuleReader.h"
#include "PEImportReader.h"
#include "ProjectPrinter.h" 

#include <iostream>
#include <string>

int wmain(int argc, wchar_t* argv[]) {
    if (argc != 2) {
        std::wcout << L"Vui long nhap PID hoac Ten tien trinh.\n";
        return 1;
    }

    std::wstring input = argv[1];
    bool isNumeric = true;
    for (wchar_t c : input) {
        if (!iswdigit(c)) isNumeric = false;
    }

    SearchResult found = isNumeric ? FindByPid(std::stoi(input)) : FindByName(input);

    if (found.processes.empty()) {
        std::wcout << L"Khong tim thay tien trinh.\n";
        return 1;
    }

    for (const auto& entry : found.processes) {
        ProcessReport report;
        report.process = entry;
        std::wstring error;

        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, entry.pid);
        if (process != nullptr) {
            BasicProcessInfo basic;
            if (ReadBasicProcessInfo(process, basic, error)) {
                report.architecture = basic.architecture;
                report.path = basic.path;

                if (!ReadPebCommandLine(process, basic.wow64, report.commandLine, error)) {
                    std::wcout << L"[!] Loi doc Command Line PID " << entry.pid << L": " << error << L"\n";
                }
            }
            else {
                std::wcout << L"[!] Loi doc Architecture/Path PID " << entry.pid << L": " << error << L"\n";
            }
            CloseHandle(process);
        }
        else {
            std::wcout << L"[!] KHONG MO DUOC PID " << entry.pid << L", Ma loi Windows: " << GetLastError() << L"\n";
        }

        ReadThreads(entry.pid, report.threads, error);
        ReadModules(entry.pid, report.modules, error);

        if (report.path != L"unavailable" && report.path != L"unknown" && !report.path.empty()) {
            if (!ReadPeImports(report.path, report.imports, error)) {
                std::wcout << L"[!] Loi doc PE Import PID " << entry.pid << L": " << error << L"\n";
            }
        }

        std::wstring text = FormatReport(report);
        std::wcout << text << L"\n";
        SaveReport(report, text, error);
    }
    return 0;
}
