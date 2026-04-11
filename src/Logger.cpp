#include "Logger.h"

Logger::Logger(const std::string& filename)
{
    file.open(filename, std::ios::app);

    if (file.tellp() == 0)
    {
        file << "CPU%,RAM%,RAM_Used_Bytes\n";
    }
}

Logger::~Logger()
{
    if (file.is_open())
        file.close();
}

void Logger::log(double cpuPercent, double ramPercent, uint64_t ramUsedBytes)
{
    file << cpuPercent << ","
         << ramPercent << ","
         << ramUsedBytes << "\n";
}

void Logger::logAlert(const Alert& alert)
{
    file << "ALERT,"
         << alert.metric << ","
         << alert.message << ","
         << alert.value << ","
         << alert.reference << "\n";
}