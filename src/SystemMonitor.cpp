#include <iostream>
#include <windows.h>
#include <iomanip>

#include "SystemMonitor.h"
#include "ConfigManager.h"

void SystemMonitor::warmUp()
{
    cpu.getUsage();
    Sleep(1000);
}

SystemMonitor::SystemMonitor()
    : config("config.json"),
      logger(config.logFile()),
      detector(
          config.cpuThreshold(),
          config.ramThreshold(),
          config.cpuSpikeThreshold(),
          config.ramSpikeThreshold(),
          config.cpuLeakThreshold(),
          config.ramLeakThreshold(),
          config.historySize()
      )
{
}

void SystemMonitor::run()
{
    warmUp();

    while (true)
    {
        system("cls");

        auto mem = memory.getStatus();
        double cpuPercent = cpu.getUsage();

        std::cout << std::fixed << std::setprecision(2);

        std::cout << "CPU Usage: " << cpuPercent << " %\n\n";

        std::cout << "Memory Usage: " << mem.percentUsed << " %\n";
        std::cout << "Total: " << mem.totalBytes / 1024.0 / 1024.0 / 1024.0 << " GB\n";

        std::cout << "Available: " << mem.availableBytes / 1024.0 / 1024.0 / 1024.0 << " GB\n";

        std::cout << "Used: " << mem.usedBytes / 1024.0 / 1024.0 / 1024.0 << " GB\n";

        logger.log(cpuPercent, mem.percentUsed, mem.usedBytes);

        auto alerts = detector.check(cpuPercent, mem.percentUsed);

        for (const auto& alert : alerts)
        {
            std::cout << alert << "\n";
        }

        Sleep(1000);
    }
}