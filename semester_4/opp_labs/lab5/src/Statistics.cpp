#include "Statistics.h"

#include <algorithm>
#include <numeric>

double Statistics::computeIterationLif(const std::vector<long long>& loads) {
    long long sumLoad = std::accumulate(loads.begin(), loads.end(), 0LL);
    long long maxLoad = *std::max_element(loads.begin(), loads.end());

    if (sumLoad == 0) {
        return 1.0;
    }

    double avgLoad = static_cast<double>(sumLoad)
                   / static_cast<double>(loads.size());

    return static_cast<double>(maxLoad) / avgLoad;
}
