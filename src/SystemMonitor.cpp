#include <iostream>
#include <windows.h>
#include <iomanip>

#include "SystemMonitor.h"

void SystemMonitor::warmUp()
{
    cpu.getUsage();
    Sleep(1000);
}

SystemMonitor::SystemMonitor()
    : logger("logs/system_log.csv"),
      detector(
          80.0,   // CPU threshold
          80.0,   // RAM threshold
          10.0,   // CPU spike threshold
          10.0,   // RAM spike threshold
          5       // History size
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