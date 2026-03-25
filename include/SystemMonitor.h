#pragma once

#include "CpuMonitor.h"
#include "MemoryMonitor.h"
#include "Logger.h"

class SystemMonitor {
public:
    SystemMonitor();
    void run();

private:
    CpuMonitor cpu;
    MemoryMonitor memory;
    Logger logger;

    void warmUp();
};