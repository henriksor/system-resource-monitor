#pragma once

#include "CpuMonitor.h"
#include "MemoryMonitor.h"
#include "Logger.h"
#include "AnomalyDetector.h"

class SystemMonitor {
public:
    SystemMonitor();
    void run();

private:
    CpuMonitor cpu;
    MemoryMonitor memory;
    Logger logger;
    AnomalyDetector detector;

    void warmUp();
};