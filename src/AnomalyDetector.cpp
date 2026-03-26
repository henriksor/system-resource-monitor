#include "AnomalyDetector.h"

std::vector<std::string> AnomalyDetector::check(double cpuPercent, double ramPercent)
{
    std::vector<std::string> alerts;

    // Vanlige terskler
    if (ramPercent > ramThreshold)
        alerts.push_back("[WARNING] High RAM usage");

    if (cpuPercent > cpuThreshold)
        alerts.push_back("[WARNING] High CPU usage");

    // Spike detection
    if (previousCpu >= 0)
    {
        double cpuDiff = cpuPercent - previousCpu;

        if (cpuDiff > cpuSpikeThreshold)
            alerts.push_back("[SPIKE] CPU usage increased rapidly");
    }

    if (previousRam >= 0)
    {
        double ramDiff = ramPercent - previousRam;

        if (ramDiff > ramSpikeThreshold)
            alerts.push_back("[SPIKE] RAM usage increased rapidly");
    }

    previousCpu = cpuPercent;
    previousRam = ramPercent;

    return alerts;
}