#include "TaskGenerator.h"

#include <algorithm>
#include <cmath>

std::vector<Task> TaskGenerator::generateGlobalTasks(
    const Config& cfg,
    int iteration
) {
    std::vector<Task> tasks;
    tasks.reserve(cfg.tasksPerIteration);

    for (int taskId = 0; taskId < cfg.tasksPerIteration; ++taskId) {
        int coef = taskWeightCoefficient(cfg, iteration, taskId);
        tasks.push_back(Task{taskId, cfg.baseRepeat * coef});
    }

    return tasks;
}

std::vector<Task> TaskGenerator::selectInitialTasks(
    const std::vector<Task>& globalTasks,
    const Config& cfg,
    int rank,
    int size
) {
    std::vector<Task> localTasks;

    if (cfg.scenario == "one") {
        if (rank == 0) {
            return globalTasks;
        }

        return localTasks;
    }

    for (const Task& task : globalTasks) {
        int virtualOwner = task.id % cfg.virtualSlots;
        int realOwner = (virtualOwner * size) / cfg.virtualSlots;

        if (realOwner == rank) {
            localTasks.push_back(task);
        }
    }

    return localTasks;
}

int TaskGenerator::taskWeightCoefficient(
    const Config& cfg,
    int iteration,
    int taskId
) {
    if (cfg.scenario == "uniform") {
        return 1;
    }

    if (cfg.scenario == "one") {
        return 1;
    }

    int virtualOwner = taskId % cfg.virtualSlots;
    int peak = iteration % cfg.virtualSlots;
    int dist = circularDistance(virtualOwner, peak, cfg.virtualSlots);

    return std::max(1, cfg.pyramidHeight - dist);
}

int TaskGenerator::circularDistance(int a, int b, int modulo) {
    int direct = std::abs(a - b);
    return std::min(direct, modulo - direct);
}
