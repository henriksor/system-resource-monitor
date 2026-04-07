#pragma once
#include <fstream>
#include <string>
#include <cstdint>

class Logger {
public:
    Logger(const std::string& filename);
    ~Logger();

    void log(double cpuPercent, 
            double ramPercent, 
            uint64_t ramUsedBytes);

private:
    std::ofstream file;
    std::string getTimestamp() const;
};