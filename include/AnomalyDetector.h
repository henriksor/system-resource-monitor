#pragma once
#include <string>

class AnomalyDetector {
public:
    std::string check(double cpuPercent, double ramPercent) const;

private:
    const double cpuThreshold = 85.0;
    const double ramThreshold = 90.0;
};