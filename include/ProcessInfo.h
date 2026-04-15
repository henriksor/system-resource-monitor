#pragma once

#include <string>
#include <cstdint>
#include <windows.h>

struct ProcessInfo {
    std::string name;
    DWORD pid;
    uint64_t ramBytes;
    double cpuPercent = 0.0;
};