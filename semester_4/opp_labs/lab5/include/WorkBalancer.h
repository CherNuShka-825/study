#pragma once

#include "Config.h"
#include "Statistics.h"
#include "Task.h"
#include "TaskQueue.h"

class WorkBalancer {
public:
    static void serverLoop(
        TaskQueue& queue,
        const Config& cfg,
        int rank
    );

    static IterationStats runWorker(
        TaskQueue& queue,
        bool balance,
        int rank,
        int size
    );

private:
    static bool tryStealWork(
        TaskQueue& queue,
        int rank,
        int size
    );

    static double executeTask(const Task& task);
};
