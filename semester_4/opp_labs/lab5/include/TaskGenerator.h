#pragma once

#include "Config.h"
#include "Task.h"

#include <vector>

class TaskGenerator {
public:
    static std::vector<Task> generateGlobalTasks(
        const Config& cfg,
        int iteration
    );

    static std::vector<Task> selectInitialTasks(
        const std::vector<Task>& globalTasks,
        const Config& cfg,
        int rank,
        int size
    );

private:
    static int taskWeightCoefficient(
        const Config& cfg,
        int iteration,
        int taskId
    );

    static int circularDistance(int a, int b, int modulo);
};
