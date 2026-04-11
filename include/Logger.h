#pragma once

#include <fstream>
#include <string>
#include <cstdint>

#include "Alert.h"

class Logger {
public:
    explicit Logger(const std::string& filename);
    ~Logger();

    void log(double cpuPercent, double ramPercent, uint64_t ramUsedBytes);
    void logAlert(const Alert& alert);

private:
    std::ofstream file;
};