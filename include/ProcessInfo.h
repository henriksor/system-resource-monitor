#pragma once

#include <string>
#include <cstdint>
#include <windows.h>

struct ProcessInfo
{
    DWORD pid;
    std::string name;
    uint64_t ramBytes;
    double cpuPercent = 0.0;   // ← NY
};