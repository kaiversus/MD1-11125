#pragma once
#include "ProjectData.h"

bool ReadPeImports(const std::wstring& path, std::vector<ImportInfo>& out, std::wstring& error);