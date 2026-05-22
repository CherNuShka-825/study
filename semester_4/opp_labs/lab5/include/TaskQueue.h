#pragma once

#include "Task.h"

#include <deque>
#include <mutex>
#include <vector>

class TaskQueue {
public:
    void setTasks(const std::vector<Task>& tasks);

    bool pop(Task& task);

    void pushMany(const std::vector<Task>& tasks);

    std::vector<Task> stealHalf(int minTasksToKeep);

    int size() const;

private:
    mutable std::mutex mutex_;
    std::deque<Task> queue_;
};
