#pragma once
#include "ProjectData.h"

std::wstring FormatReport(const ProcessReport& report);
bool SaveReport(const ProcessReport& report, const std::wstring& text, std::wstring& error);