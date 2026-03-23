#pragma once

#include "CpuMonitor.h"
#include "MemoryMonitor.h"

class SystemMonitor {
public:
    void run();

private:
    CpuMonitor cpu;
    MemoryMonitor memory;

    void warmUp();
};