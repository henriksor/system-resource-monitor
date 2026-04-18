#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "Alert.h"
#include "ProcessInfo.h"

struct SystemSnapshot {
    struct MemoryDetails {
        uint64_t usedBytes = 0;
        uint64_t totalBytes = 0;
        uint64_t availableBytes = 0;
    };

    std::optional<double> cpuPercent;
    std::optional<double> ramPercent;
    std::optional<MemoryDetails> memory;
    std::vector<ProcessInfo> processes;
    std::vector<Alert> alerts;
};
