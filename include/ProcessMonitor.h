#pragma once

#include <vector>
#include "ProcessInfo.h"

class ProcessMonitor {
public:
    std::vector<ProcessInfo> getProcesses();
};