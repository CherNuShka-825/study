#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <mpi.h>

using namespace std;

int getLocalRows(int N, int size, int rank) {
    int base = N / size;
    int rem = N % size;
    return base + (rank < rem ? 1 : 0);
}

int getRowStart(int N, int size, int rank) {
    int base = N / size;
    int rem = N % size;
    return rank * base + min(rank, rem);
}

void buildCountsAndDispls(int N, int size, vector<int> &counts, vector<int> &displs) {
    counts.resize(size);
    displs.resize(size);

    for (int rank = 0; rank < size; ++rank) {
        counts[rank] = getLocalRows(N, size, rank);
        displs[rank] = getRowStart(N, size, rank);
    }
}

void fillLocalMat(vector<double> &localA, int N, int localRows, int rowStart) {
    for (int i = 0; i < localRows; ++i) {
        int globalRow = rowStart + i;
        for (int j = 0; j < N; ++j) {
            if (globalRow == j) {
                localA[i * N + j] = 2.0;
            } else {
                localA[i * N + j] = 1.0;
            }
        }
    }
}

double vectorNorm(const vector<double> &v) {
    double sum = 0.0;
    for (double value : v) {
        sum += value * value;
    }
    return sqrt(sum);
}

void multiplyLocalMatrixVector(
    const vector<double> &localA,
    const vector<double> &x,
    vector<double> &localResult,
    int localRows,
    int N
) {
    for (int i = 0; i < localRows; ++i) {
        localResult[i] = 0.0;
        for (int j = 0; j < N; ++j) {
            localResult[i] += localA[i * N + j] * x[j];
        }
    }
}

vector<double> simpleIterationMethodMPIv1(
    const vector<double> &localA,
    const vector<double> &b,
    int N,
    int localRows,
    int rowStart,
    double tau,
    double epsilon,
    int maxIterations,
    MPI_Comm comm,
    double &seqTime
) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    vector<int> counts, displs;
    buildCountsAndDispls(N, size, counts, displs);

    vector<double> x(N, 0.0);
    vector<double> xLocalNew(localRows, 0.0);

    // Малый последовательный участок: вычисление нормы b
    double tSeqStart = MPI_Wtime();
    double normB = vectorNorm(b);
    if (normB == 0.0) {
        normB = 1.0;
    }
    seqTime += MPI_Wtime() - tSeqStart;

    vector<double> localAx(localRows);

    for (int iter = 0; iter < maxIterations; ++iter) {
        // Параллельная вычислительная часть
        multiplyLocalMatrixVector(localA, x, localAx, localRows, N);

        double localResidualSum = 0.0;

        // Параллельная вычислительная часть
        for (int i = 0; i < localRows; ++i) {
            int globalRow = rowStart + i;
            double residual = localAx[i] - b[globalRow];

            localResidualSum += residual * residual;
            xLocalNew[i] = x[globalRow] - tau * residual;
        }

        double globalResidualSum = 0.0;

        // MPI-коммуникация
        MPI_Allreduce(
            &localResidualSum,
            &globalResidualSum,
            1,
            MPI_DOUBLE,
            MPI_SUM,
            comm
        );

        // Малый последовательный участок:
        // вычисление критерия и проверка условия остановки
        tSeqStart = MPI_Wtime();
        double criterion = sqrt(globalResidualSum) / normB;
        bool converged = (criterion < epsilon);
        seqTime += MPI_Wtime() - tSeqStart;

        // MPI-коммуникация
        MPI_Allgatherv(
            xLocalNew.data(),
            localRows,
            MPI_DOUBLE,
            x.data(),
            counts.data(),
            displs.data(),
            MPI_DOUBLE,
            comm
        );

        if (converged) {
            if (rank == 0) {
                // Малый последовательный участок на rank 0: вывод
                tSeqStart = MPI_Wtime();
                cout << "Converged in " << iter << " iterations.\n";
                seqTime += MPI_Wtime() - tSeqStart;
            }
            return x;
        }
    }

    if (rank == 0) {
        double tSeqStart = MPI_Wtime();
        cout << "Maximum number of iterations reached.\n";
        seqTime += MPI_Wtime() - tSeqStart;
    }

    return x;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int N = 19000;
    double tau = 0.0001;
    double epsilon = 1e-5;
    int maxIterations = 1000000;

    int localRows = getLocalRows(N, size, rank);
    int rowStart = getRowStart(N, size, rank);

    vector<double> localA(localRows * N);

    fillLocalMat(localA, N, localRows, rowStart);

    vector<double> b(N, static_cast<double>(N + 1));

    double seqTime = 0.0;
    double startTime = MPI_Wtime();

    vector<double> x = simpleIterationMethodMPIv1(
        localA,
        b,
        N,
        localRows,
        rowStart,
        tau,
        epsilon,
        maxIterations,
        MPI_COMM_WORLD,
        seqTime
    );

    double endTime = MPI_Wtime();
    double totalTime = endTime - startTime;

    auto max_diff = 0.0;
    for (auto xi : x) {
        max_diff = max(max_diff, abs(1.0 - xi));
    }

    // Для корректной оценки берём максимум по процессам
    double maxTotalTime = 0.0;
    double maxSeqTime = 0.0;

    MPI_Reduce(&totalTime, &maxTotalTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&seqTime, &maxSeqTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        cout << fixed << setprecision(10);
        cout << "Elapsed time: " << maxTotalTime << " sec\n";
        cout << "Estimated tiny sequential time: " << maxSeqTime << " sec\n";
        cout << "Estimated tiny sequential fraction: "
             << (maxSeqTime / maxTotalTime) * 100.0 << " %\n";
        cout << "max_diff = " << max_diff << "\n";
        cout << "good answer? " << ((max_diff < epsilon) ? "Yes" : "No") << "\n";
    }

    MPI_Finalize();
    return 0;
}