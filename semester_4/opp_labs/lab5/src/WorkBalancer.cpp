#include "WorkBalancer.h"

#include "MpiTags.h"

#include <mpi.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

static void dieIfMpiError(int code, const std::string& message) {
    if (code != MPI_SUCCESS) {
        throw std::runtime_error(message);
    }
}

static std::vector<int> serializeTasks(const std::vector<Task>& tasks) {
    std::vector<int> data;
    data.reserve(tasks.size() * 2);

    for (const Task& task : tasks) {
        data.push_back(task.id);
        data.push_back(task.repeatNum);
    }

    return data;
}

static std::vector<Task> deserializeTasks(const std::vector<int>& data) {
    std::vector<Task> tasks;
    tasks.reserve(data.size() / 2);

    for (std::size_t i = 0; i + 1 < data.size(); i += 2) {
        tasks.push_back(Task{data[i], data[i + 1]});
    }

    return tasks;
}

static std::vector<int> makeVictimOrder(int rank, int size) {
    std::vector<int> victims;
    victims.reserve(size - 1);

    for (int distance = 1; distance < size; ++distance) {
        int plus = (rank + distance) % size;
        int minus = (rank - distance + size) % size;

        if (plus != rank &&
            std::find(victims.begin(), victims.end(), plus) == victims.end()) {
            victims.push_back(plus);
        }

        if (minus != rank &&
            std::find(victims.begin(), victims.end(), minus) == victims.end()) {
            victims.push_back(minus);
        }
    }

    return victims;
}

double WorkBalancer::executeTask(const Task& task) {
    double result = 0.0;

    for (int i = 1; i <= task.repeatNum; ++i) {
        result += std::sqrt(static_cast<double>(i));
    }

    return result;
}

bool WorkBalancer::tryStealWork(
    TaskQueue& queue,
    int rank,
    int size
) {
    if (size == 1) {
        return false;
    }

    int dummy = 0;
    std::vector<int> victims = makeVictimOrder(rank, size);

    for (int target : victims) {
        dieIfMpiError(
            MPI_Send(
                &dummy,
                1,
                MPI_INT,
                target,
                TAG_WORK_REQUEST,
                MPI_COMM_WORLD
            ),
            "MPI_Send WORK_REQUEST failed"
        );

        int count = 0;

        dieIfMpiError(
            MPI_Recv(
                &count,
                1,
                MPI_INT,
                target,
                TAG_WORK_COUNT,
                MPI_COMM_WORLD,
                MPI_STATUS_IGNORE
            ),
            "MPI_Recv WORK_COUNT failed"
        );

        if (count == 0) {
            continue;
        }

        std::vector<int> packed(static_cast<std::size_t>(count) * 2);

        dieIfMpiError(
            MPI_Recv(
                packed.data(),
                static_cast<int>(packed.size()),
                MPI_INT,
                target,
                TAG_WORK_DATA,
                MPI_COMM_WORLD,
                MPI_STATUS_IGNORE
            ),
            "MPI_Recv WORK_DATA failed"
        );

        queue.pushMany(deserializeTasks(packed));

        return true;
    }

    return false;
}

void WorkBalancer::serverLoop(
    TaskQueue& queue,
    const Config& cfg,
    int rank
) {
    while (true) {
        int dummy = 0;
        MPI_Status status{};

        dieIfMpiError(
            MPI_Recv(
                &dummy,
                1,
                MPI_INT,
                MPI_ANY_SOURCE,
                TAG_WORK_REQUEST,
                MPI_COMM_WORLD,
                &status
            ),
            "Server MPI_Recv WORK_REQUEST failed"
        );

        if (status.MPI_SOURCE == rank) {
            break;
        }

        std::vector<Task> tasksToGive = queue.stealHalf(cfg.minTasksToKeep);

        int count = static_cast<int>(tasksToGive.size());

        dieIfMpiError(
            MPI_Send(
                &count,
                1,
                MPI_INT,
                status.MPI_SOURCE,
                TAG_WORK_COUNT,
                MPI_COMM_WORLD
            ),
            "Server MPI_Send WORK_COUNT failed"
        );

        if (count > 0) {
            std::vector<int> packed = serializeTasks(tasksToGive);

            dieIfMpiError(
                MPI_Send(
                    packed.data(),
                    static_cast<int>(packed.size()),
                    MPI_INT,
                    status.MPI_SOURCE,
                    TAG_WORK_DATA,
                    MPI_COMM_WORLD
                ),
                "Server MPI_Send WORK_DATA failed"
            );
        }
    }
}

IterationStats WorkBalancer::runWorker(
    TaskQueue& queue,
    bool balance,
    int rank,
    int size
) {
    IterationStats stats;
    Task task{};

    while (true) {
        if (queue.pop(task)) {
            stats.localResult += executeTask(task);
            stats.localLoad += task.repeatNum;
            stats.localTasksDone += 1;
            continue;
        }

        if (!balance) {
            break;
        }

        bool stolen = tryStealWork(queue, rank, size);

        if (!stolen) {
            break;
        }
    }

    return stats;
}
