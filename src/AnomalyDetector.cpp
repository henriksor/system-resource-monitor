#include "AnomalyDetector.h"

AnomalyDetector::AnomalyDetector(double cpuThreshold,
                                 double ramThreshold,
                                 double cpuSpikeThreshold,
                                 double ramSpikeThreshold,
                                 double cpuLeakThreshold,
                                 double ramLeakThreshold,
                                 size_t historySize)
    : cpuThreshold(cpuThreshold),
      ramThreshold(ramThreshold),
      cpuSpikeThreshold(cpuSpikeThreshold),
      ramSpikeThreshold(ramSpikeThreshold),
      cpuLeakThreshold(cpuLeakThreshold),
      ramLeakThreshold(ramLeakThreshold),
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

bool AnomalyDetector::isMonotonicIncreasing(
    const std::vector<double>& values) const
{
    if (values.size() < 2)
        return false;

    for (size_t i = 1; i < values.size(); ++i)
    {
        if (values[i] <= values[i - 1])
            return false;
    }

    return true;
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

    // leak detection
    if (cpuHistory.size() == maxHistory)
    {
        double increase = cpuHistory.back() - cpuHistory.front();

        if (increase > cpuLeakThreshold &&
            isMonotonicIncreasing(cpuHistory))
        {
            alerts.push_back("[LEAK] CPU shows steady increase");
        }
    }

    if (ramHistory.size() == maxHistory)
    {
        double increase = ramHistory.back() - ramHistory.front();

        if (increase > ramLeakThreshold &&
            isMonotonicIncreasing(ramHistory))
        {
            alerts.push_back("[LEAK] RAM shows steady increase");
        }
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