#pragma once
#include <cstdint>

class MemoryMonitor {
public:
    struct MemoryStatus {
        uint64_t totalBytes;
        uint64_t availableBytes;
        uint64_t usedBytes;
        double percentUsed;
    };

    MemoryStatus getStatus() const;
};