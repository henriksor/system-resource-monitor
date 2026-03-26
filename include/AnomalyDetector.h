#pragma once
#include <string>
#include <vector>

class AnomalyDetector {
public:
    AnomalyDetector(double cpuThreshold, double ramThreshold);
    std::vector<std::string> check(double cpuPercent, double ramPercent) const;

private:
    const double cpuThreshold;
    const double ramThreshold;
};