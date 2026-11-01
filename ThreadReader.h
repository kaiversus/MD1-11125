#pragma once
#include "ProjectData.h"

bool ReadThreads(DWORD pid, std::vector<ThreadInfo>& out, std::wstring& error);