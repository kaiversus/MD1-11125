#include "ProjectPrinter.h"
#include <sstream>
#include <fstream>
#include <filesystem>

std::wstring WidenAscii(const std::string& text) {
    return std::wstring(text.begin(), text.end());
}

std::wstring FormatReport(const ProcessReport& r) {
    std::wostringstream out;
    out << L"Process Information:\n";
    out << L"- Name: " << r.process.name << L'\n';
    out << L"- PID: " << r.process.pid << L'\n';
    out << L"- PPID: " << r.process.ppid << L'\n';
    out << L"- Architecture: " << r.architecture << L'\n';
    out << L"- Path: " << r.path << L'\n';
    out << L"- Command Line: " << r.commandLine << L'\n';

    out << L"\nThread Information:\n";
    for (const auto& t : r.threads) {
        out << L"- Thread ID: " << t.id << L", State: " << t.state << L'\n';
    }

    out << L"\nDLL Information:\n";
    for (const auto& m : r.modules) {
        out << L"- DLL Name: " << m.name << L", Path: " << m.path << L'\n';
    }

    out << L"\nPE Import Information:\n";
    for (const auto& dll : r.imports) {
        out << L"- DLL Name: " << WidenAscii(dll.dll) << L", Imported Functions: ";
        for (size_t i = 0; i < dll.functions.size(); ++i) {
            if (i > 0) out << L", ";
            out << WidenAscii(dll.functions[i]);
        }
        out << L'\n';
    }
    return out.str();
}

bool SaveReport(const ProcessReport& report, const std::wstring& text, std::wstring& error) {
    error.clear();
    std::wstring filename = report.process.name + L"-" + std::to_wstring(report.process.pid) + L".txt";

    int bytes = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (bytes == 0) { error = L"Loi chuyen doi UTF-8"; return false; }

    std::string utf8(bytes, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data(), bytes, nullptr, nullptr);

    std::ofstream file(std::filesystem::path(filename), std::ios::binary);
    if (!file) { error = L"Khong the mo file output"; return false; }

    file.write(utf8.data(), utf8.size());
    return true;
}