#include "TaskQueue.h"
#include "Task.h"
#include <algorithm>

void TaskQueue::setTasks(const std::vector<Task>& tasks) {
    std::lock_guard lock(mutex_);

    queue_.clear();

    for (const Task& task : tasks) {
        queue_.push_back(task);
    }
}

bool TaskQueue::pop(Task& task) {
    std::lock_guard lock(mutex_);

    if (queue_.empty()) {
        return false;
    }

    task = queue_.front();
    queue_.pop_front();

    return true;
}

void TaskQueue::pushMany(const std::vector<Task>& tasks) {
    std::lock_guard lock(mutex_);

    for (const Task& task : tasks) {
        queue_.push_back(task);
    }
}

std::vector<Task> TaskQueue::stealHalf(int minTasksToKeep) {
    std::lock_guard lock(mutex_);

    if (static_cast<int>(queue_.size()) <= minTasksToKeep) {
        return {};
    }

    int availableToGive = static_cast<int>(queue_.size()) - minTasksToKeep;
    int giveCount = std::min(availableToGive, static_cast<int>(queue_.size()) / 2);

    std::vector<Task> stolen;
    stolen.reserve(giveCount);

    for (int i = 0; i < giveCount; ++i) {
        stolen.push_back(queue_.back());
        queue_.pop_back();
    }

    return stolen;
}

int TaskQueue::size() const {
    std::lock_guard lock(mutex_);
    return static_cast<int>(queue_.size());
}