#include "Config.h"
#include "MpiTags.h"
#include "Statistics.h"
#include "Task.h"
#include "TaskGenerator.h"
#include "TaskQueue.h"
#include "WorkBalancer.h"

#include <mpi.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

static Config parseArgs(int argc, char** argv) {
    Config cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        auto requireValue = [&](const std::string& option) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing value for " + option);
            }

            return argv[++i];
        };

        if (arg == "--balance") {
            std::string value = requireValue(arg);
            if (value == "on") {
                cfg.balance = true;
            } else if (value == "off") {
                cfg.balance = false;
            } else {
                throw std::runtime_error("--balance must be on or off");
            }

        } else if (arg == "--scenario") {
            std::string scenario = requireValue(arg);
            if (scenario != "pyramid" &&
                scenario != "uniform" &&
                scenario != "one") {
                throw std::runtime_error(
                    "--scenario must be pyramid, uniform or one"
                );
                }
            cfg.scenario = scenario;

        } else if (arg == "--iterations") {
            cfg.iterations = std::stoi(requireValue(arg));

        } else if (arg == "--tasks") {
            cfg.tasksPerIteration = std::stoi(requireValue(arg));

        } else if (arg == "--base-repeat") {
            cfg.baseRepeat = std::stoi(requireValue(arg));

        } else if (arg == "--virtual-slots") {
            cfg.virtualSlots = std::stoi(requireValue(arg));

        } else if (arg == "--pyramid-height") {
            cfg.pyramidHeight = std::stoi(requireValue(arg));

        } else if (arg == "--min-keep") {
            cfg.minTasksToKeep = std::stoi(requireValue(arg));

        } else if (arg == "--help") {
            std::cout
                << "Usage:\n"
                << "  mpirun -np N ./main [options]\n\n"
                << "Options:\n"
                << "  --balance on|off\n"
                << "  --scenario pyramid|uniform|one\n"
                << "  --iterations N\n"
                << "  --tasks N\n"
                << "  --base-repeat N\n"
                << "  --virtual-slots N\n"
                << "  --pyramid-height N\n"
                << "  --min-keep N\n";

            std::exit(0);

        } else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }

    if (cfg.iterations <= 0 ||
        cfg.tasksPerIteration <= 0 ||
        cfg.baseRepeat <= 0) {
        throw std::runtime_error(
            "iterations, tasks and base-repeat must be positive"
        );
    }

    if (cfg.virtualSlots <= 0 ||
        cfg.pyramidHeight <= 0 ||
        cfg.minTasksToKeep < 0) {
        throw std::runtime_error(
            "virtual-slots and pyramid-height must be positive, "
            "min-keep must be non-negative"
        );
    }

    return cfg;
}

int main(int argc, char** argv) {
    int provided = 0;

    MPI_Init_thread(
        &argc,
        &argv,
        MPI_THREAD_MULTIPLE,
        &provided
    );

    int rank = 0;
    int size = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    try {
        if (provided < MPI_THREAD_MULTIPLE) {
            if (rank == 0) {
                std::cerr
                    << "Error: MPI implementation does not provide "
                    << "MPI_THREAD_MULTIPLE\n";
            }

            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        Config cfg = parseArgs(argc, argv);

        if (size > cfg.virtualSlots && rank == 0) {
            std::cerr
                << "Warning: size > virtualSlots. "
                << "Some ranks may receive no initial tasks. "
                << "Use --virtual-slots >= max process count.\n";
        }

        TaskQueue queue;

        long long totalLocalLoad = 0;
        long long totalLocalTasksDone = 0;
        double totalLocalResult = 0.0;

        double lifSum = 0.0;

        MPI_Barrier(MPI_COMM_WORLD);

        double startTime = MPI_Wtime();

        for (int iteration = 0; iteration < cfg.iterations; ++iteration) {
            std::vector<Task> globalTasks = TaskGenerator::generateGlobalTasks(cfg, iteration);

            std::vector<Task> localTasks =
                TaskGenerator::selectInitialTasks(
                    globalTasks,
                    cfg,
                    rank,
                    size
                );

            queue.setTasks(localTasks);

            std::thread serverThread;

            if (cfg.balance) {
                serverThread = std::thread(
                    WorkBalancer::serverLoop,
                    std::ref(queue),
                    std::cref(cfg),
                    rank
                );
            }

            IterationStats iterStats =
                WorkBalancer::runWorker(
                    queue,
                    cfg.balance,
                    rank,
                    size
                );

            MPI_Barrier(MPI_COMM_WORLD);

            if (cfg.balance) {
                int dummy = 0;

                MPI_Send(
                    &dummy,
                    1,
                    MPI_INT,
                    rank,
                    TAG_WORK_REQUEST,
                    MPI_COMM_WORLD
                );

                serverThread.join();
            }

            totalLocalLoad += iterStats.localLoad;
            totalLocalTasksDone += iterStats.localTasksDone;
            totalLocalResult += iterStats.localResult;

            std::vector<long long> loads;

            if (rank == 0) {
                loads.resize(size);
            }

            MPI_Gather(
                &iterStats.localLoad,
                1,
                MPI_LONG_LONG,
                rank == 0 ? loads.data() : nullptr,
                1,
                MPI_LONG_LONG,
                0,
                MPI_COMM_WORLD
            );

            if (rank == 0) {
                double iterationLif =
                    Statistics::computeIterationLif(loads);

                lifSum += iterationLif;
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);

        double finishTime = MPI_Wtime();
        double localTime = finishTime - startTime;

        double maxTime = 0.0;
        long long globalTasksDone = 0;
        long long globalLoad = 0;
        double globalResult = 0.0;

        MPI_Reduce(
            &localTime,
            &maxTime,
            1,
            MPI_DOUBLE,
            MPI_MAX,
            0,
            MPI_COMM_WORLD
        );

        MPI_Reduce(
            &totalLocalTasksDone,
            &globalTasksDone,
            1,
            MPI_LONG_LONG,
            MPI_SUM,
            0,
            MPI_COMM_WORLD
        );

        MPI_Reduce(
            &totalLocalLoad,
            &globalLoad,
            1,
            MPI_LONG_LONG,
            MPI_SUM,
            0,
            MPI_COMM_WORLD
        );

        MPI_Reduce(
            &totalLocalResult,
            &globalResult,
            1,
            MPI_DOUBLE,
            MPI_SUM,
            0,
            MPI_COMM_WORLD
        );

        if (rank == 0) {
            double avgLif = lifSum / static_cast<double>(cfg.iterations);

            std::string balanceName = cfg.balance ? "on" : "off";

            std::cout
                << "RESULT "
                << "balance=" << balanceName << ' '
                << "scenario=" << cfg.scenario << ' '
                << "processes=" << size << ' '
                << "iterations=" << cfg.iterations << ' '
                << "tasks_per_iteration=" << cfg.tasksPerIteration << ' '
                << "time=" << maxTime << ' '
                << "avg_lif=" << avgLif << ' '
                << "total_tasks_done=" << globalTasksDone << ' '
                << "total_load=" << globalLoad << ' '
                << "checksum=" << globalResult
                << '\n';
        }

    } catch (const std::exception& ex) {
        std::cerr
            << "Rank "
            << rank
            << " error: "
            << ex.what()
            << '\n';

        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Finalize();

    return 0;
}