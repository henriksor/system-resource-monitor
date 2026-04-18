#pragma once
#include <cstdint>
#include <optional>

class MemoryMonitor {
public:
    struct MemoryStatus {
        uint64_t totalBytes;
        uint64_t availableBytes;
        uint64_t usedBytes;
        double percentUsed;
    };

    std::optional<MemoryStatus> getStatus() const;
};
