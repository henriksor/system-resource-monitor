#include "SystemMonitor.h"
#include <exception>
#include <iostream>

int main()
{
    try
    {
        SystemMonitor monitor;
        monitor.run();
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to start system monitor: "
                  << ex.what() << "\n";
        return 1;
    }
}
