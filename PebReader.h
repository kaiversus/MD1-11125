#pragma once
#include <windows.h>
#include <string>

bool ReadPebCommandLine(HANDLE process, bool wow64, std::wstring& commandLine, std::wstring& error);