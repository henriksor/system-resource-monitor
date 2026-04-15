#pragma once
#include <vector>
#include <unordered_map>

#include "ProcessInfo.h"

class ProcessMonitor {
    private:
    std::unordered_map<DWORD, uint64_t> previousProcessTimes;
    uint64_t previousSystemTime = 0;

    public:
        std::vector<ProcessInfo> getProcesses();
};