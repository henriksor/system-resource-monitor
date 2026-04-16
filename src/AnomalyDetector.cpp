#include "AnomalyDetector.h"
#include <algorithm>

AnomalyDetector::AnomalyDetector(
    double cpuT,
    double ramT,
    double cpuSpikeT,
    double ramSpikeT,
    double cpuLeakT,
    double ramLeakT,
    size_t history
)
    : cpuThreshold(cpuT),
      ramThreshold(ramT),
      cpuSpikeThreshold(cpuSpikeT),
      ramSpikeThreshold(ramSpikeT),
      cpuLeakThreshold(cpuLeakT),
      ramLeakThreshold(ramLeakT),
      // Leak/spike logic needs at least two samples to compare current data to
      // prior history, so clamp invalid inputs to a safe minimum.
      maxHistory(std::max<size_t>(history, 2))
{
}

std::vector<Alert> AnomalyDetector::check(double cpuPercent, double ramPercent)
{
    addToHistory(cpuPercent, ramPercent);

    std::vector<Alert> alerts;

    auto thresholdAlerts = checkThreshold(cpuPercent, ramPercent);
    auto spikeAlerts     = checkSpike(cpuPercent, ramPercent);
    auto leakAlerts      = checkLeak(cpuPercent, ramPercent);

    alerts.insert(alerts.end(), thresholdAlerts.begin(), thresholdAlerts.end());
    alerts.insert(alerts.end(), spikeAlerts.begin(), spikeAlerts.end());
    alerts.insert(alerts.end(), leakAlerts.begin(), leakAlerts.end());

    return alerts;
}

void AnomalyDetector::addToHistory(double cpuPercent, double ramPercent)
{
    cpuHistory.push_back(cpuPercent);
    ramHistory.push_back(ramPercent);

    if (cpuHistory.size() > maxHistory)
        cpuHistory.erase(cpuHistory.begin());

    if (ramHistory.size() > maxHistory)
        ramHistory.erase(ramHistory.begin());
}

std::vector<Alert> AnomalyDetector::checkThreshold(double cpuPercent, double ramPercent)
{
    std::vector<Alert> alerts;

    if (cpuPercent > cpuThreshold)
    {
        alerts.push_back({
            "CPU",
            AlertType::Threshold,
            Severity::Warning,
            cpuPercent,
            cpuThreshold,
            "CPU exceeded threshold"
        });
    }

    if (ramPercent > ramThreshold)
    {
        alerts.push_back({
            "RAM",
            AlertType::Threshold,
            Severity::Warning,
            ramPercent,
            ramThreshold,
            "RAM exceeded threshold"
        });
    }

    return alerts;
}

std::vector<Alert> AnomalyDetector::checkSpike(double cpuPercent, double ramPercent)
{
    std::vector<Alert> alerts;

    if (cpuHistory.size() >= 2)
    {
        double cpuDiff = cpuPercent - cpuHistory[cpuHistory.size() - 2];

        if (cpuDiff > cpuSpikeThreshold)
        {
            alerts.push_back({
                "CPU",
                AlertType::Spike,
                Severity::Warning,
                cpuPercent,
                cpuDiff,
                "CPU spike detected"
            });
        }
    }

    if (ramHistory.size() >= 2)
    {
        double ramDiff = ramPercent - ramHistory[ramHistory.size() - 2];

        if (ramDiff > ramSpikeThreshold)
        {
            alerts.push_back({
                "RAM",
                AlertType::Spike,
                Severity::Warning,
                ramPercent,
                ramDiff,
                "RAM spike detected"
            });
        }
    }

    return alerts;
}

std::vector<Alert> AnomalyDetector::checkLeak(double cpuPercent, double ramPercent)
{
    std::vector<Alert> alerts;

    if (cpuHistory.size() >= maxHistory)
    {
        double avgCpu = calculateAverage(cpuHistory);

        if (cpuPercent - avgCpu > cpuLeakThreshold)
        {
            alerts.push_back({
                "CPU",
                AlertType::Leak,
                Severity::Critical,
                cpuPercent,
                avgCpu,
                "CPU leak/trend detected"
            });
        }
    }

    if (ramHistory.size() >= maxHistory)
    {
        double avgRam = calculateAverage(ramHistory);

        if (ramPercent - avgRam > ramLeakThreshold)
        {
            alerts.push_back({
                "RAM",
                AlertType::Leak,
                Severity::Critical,
                ramPercent,
                avgRam,
                "RAM leak/trend detected"
            });
        }
    }

    return alerts;
}

double AnomalyDetector::calculateAverage(const std::vector<double>& values) const
{
    if (values.empty())
        return 0.0;

    double sum = 0.0;

    for (double v : values)
        sum += v;

    return sum / values.size();
}
