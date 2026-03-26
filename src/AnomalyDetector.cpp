#include "AnomalyDetector.h"

std::string AnomalyDetector::check(double cpuPercent, double ramPercent) const
{
    if (ramPercent > ramThreshold)
        return "[WARNING] High RAM usage";

    if (cpuPercent > cpuThreshold)
        return "[WARNING] High CPU usage";

    return "";
}