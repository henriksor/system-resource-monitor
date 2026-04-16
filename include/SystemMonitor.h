#pragma once

#include "CpuMonitor.h"
#include "MemoryMonitor.h"
#include "Logger.h"
#include "AnomalyDetector.h"
#include "ConfigManager.h"
#include "ProcessMonitor.h"

#include <atomic>

class SystemMonitor {
public:
    SystemMonitor();
    void run();
    void stop();

private:
    void warmUp();

    std::atomic<bool> running{true};

    ConfigManager config;
    CpuMonitor cpu;
    MemoryMonitor memory;
    ProcessMonitor processMonitor;
    Logger logger;
    AnomalyDetector detector;
};
