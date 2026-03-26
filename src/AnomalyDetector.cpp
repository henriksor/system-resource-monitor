#include "AnomalyDetector.h"

AnomalyDetector::AnomalyDetector(double cpuT, double ramT)
    : cpuThreshold(cpuT), ramThreshold(ramT)
{
}

std::vector<std::string> AnomalyDetector::check(double cpuPercent, double ramPercent) const
{
    std::vector<std::string> alerts;

    if (ramPercent > ramThreshold)
        alerts.push_back("[WARNING] High RAM usage");

    if (cpuPercent > cpuThreshold)
        alerts.push_back("[WARNING] High CPU usage");

    return alerts;
}