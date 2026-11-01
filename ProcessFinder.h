#pragma once
#include "ProjectData.h"

struct SearchResult {
    std::vector<ProcessEntry> processes;
    DWORD error = ERROR_SUCCESS;
};

SearchResult FindByName(const std::wstring& name);
SearchResult FindByPid(DWORD pid);