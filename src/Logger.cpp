#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>

Logger::Logger(const std::string& filename)
{
    file.open(filename, std::ios::out);
    file << "timestamp,cpu_percent,ram_percent,ram_used_bytes\n";
}

Logger::~Logger()
{
    if (file.is_open())
        file.close();
}

void Logger::log(double cpuPercent, 
                double ramPercent, 
                uint64_t ramUsedBytes)
{
    file << getTimestamp() << "," << cpuPercent << "," << ramPercent << "," << ramUsedBytes << "\n";
}

std::string Logger::getTimestamp() const
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::tm tm;
    localtime_s(&tm, &time);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}