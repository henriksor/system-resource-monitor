#pragma once
#include <cstdint>

class CpuMonitor {
public:
    double getUsage();

private:
    struct CpuTimes {
        uint64_t idle;
        uint64_t kernel;
        uint64_t user;
    };

    CpuTimes previous{};
    bool hasPrevious = false;

    CpuTimes getCurrentTimes();
};