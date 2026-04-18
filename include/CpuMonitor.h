#pragma once
#include <cstdint>
#include <optional>

class CpuMonitor {
public:
    std::optional<double> getUsage();

private:
    struct CpuTimes {
        uint64_t idle;
        uint64_t kernel;
        uint64_t user;
    };

    CpuTimes previous{};
    bool hasPrevious = false;

    bool getCurrentTimes(CpuTimes& times) const;
};
