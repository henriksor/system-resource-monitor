#pragma once
#include <cstddef>
#include <deque>
#include <vector>
#include <unordered_map>

#include "Alert.h"
#include "ProcessInfo.h"

struct ProcessMonitorResult
{
    std::vector<ProcessInfo> processes;
    std::vector<Alert> alerts;
};

class ProcessMonitor {
private:
    std::unordered_map<DWORD, uint64_t> previousProcessTimes;
    std::unordered_map<DWORD, std::deque<double>> cpuHistoryByPid;
    std::unordered_map<DWORD, std::deque<uint64_t>> ramHistoryByPid;
    uint64_t previousSystemTime = 0;
    double cpuSpikeThreshold;
    double ramLeakThresholdBytes;
    size_t maxHistory;

public:
    ProcessMonitor(
        double cpuSpikeThreshold,
        double ramLeakThresholdMb,
        size_t historySize
    );
    ProcessMonitorResult getProcesses();
};
