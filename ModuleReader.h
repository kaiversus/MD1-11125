#pragma once
#include "ProjectData.h"

bool ReadModules(DWORD pid, std::vector<ModuleInfo>& out, std::wstring& error);