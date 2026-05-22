#pragma once

#include <vector>

struct IterationStats {
    long long localLoad = 0;
    long long localTasksDone = 0;
    double localResult = 0.0;
};

class Statistics {
public:
    static double computeIterationLif(const std::vector<long long>& loads);
};
