#include "AnomalyDetector.h"

AnomalyDetector::AnomalyDetector(double cpuThreshold, double ramThreshold, double cpuSpikeThreshold, double ramSpikeThreshold, size_t historySize)
    : cpuThreshold(cpuThreshold),
      ramThreshold(ramThreshold),
      cpuSpikeThreshold(cpuSpikeThreshold),
      ramSpikeThreshold(ramSpikeThreshold),
      maxHistory(historySize)
{
}

double AnomalyDetector::calculateAverage(
    const std::vector<double>& values) const
{
    if (values.empty())
        return 0.0;

    double sum = 0.0;

    for (double v : values)
        sum += v;

    return sum / values.size();
}

std::vector<std::string> AnomalyDetector::check(double cpuPercent, double ramPercent)
{
    std::vector<std::string> alerts;


    // absolute threshold checks
    if (ramPercent > ramThreshold)
        alerts.push_back("[WARNING] High RAM usage");

    if (cpuPercent > cpuThreshold)
        alerts.push_back("[WARNING] High CPU usage");


    // spike detection (based on history average)
    if (!cpuHistory.empty())
    {
        double avgCpu = calculateAverage(cpuHistory);

        if ((cpuPercent - avgCpu) > cpuSpikeThreshold)
            alerts.push_back("[SPIKE] CPU deviates from recent average");
    }

    if (!ramHistory.empty())
    {
        double avgRam = calculateAverage(ramHistory);

        if ((ramPercent - avgRam) > ramSpikeThreshold)
            alerts.push_back("[SPIKE] RAM deviates from recent average");
    }

    // update history AFTER analysis
    cpuHistory.push_back(cpuPercent);
    ramHistory.push_back(ramPercent);

    if (cpuHistory.size() > maxHistory)
        cpuHistory.erase(cpuHistory.begin());

    if (ramHistory.size() > maxHistory)
        ramHistory.erase(ramHistory.begin());

    return alerts;
}