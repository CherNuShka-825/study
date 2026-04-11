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

double vectorNorm(const vector<double>& localB, MPI_Comm comm) {
    double localNormB = 0.0;

    for (double v : localB) {
        localNormB += v * v;
    }

    double globalNormB;

    MPI_Allreduce(&localNormB, &globalNormB, 1, MPI_DOUBLE, MPI_SUM, comm);

    globalNormB = sqrt(globalNormB);
    return globalNormB;
}

vector<double> multiplyLocalMatrixVector(
    const vector<double>& localA,
    const vector<double>& xLocal,
    int N,
    int localRows,
    int rowStart,
    const vector<int>& counts,
    const vector<int>& displs,
    MPI_Comm comm
) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    int left  = (rank - 1 + size) % size;
    int right = (rank + 1) % size;

    vector<double> localAx(localRows, 0.0);

    vector<double> buffer = xLocal;
    int currentOwner = rank;

    int blockSize = localRows;
    int blockStart = rowStart;

    for (int step = 0; step < size; step++) {

        for (int i = 0; i < localRows; i++) {
            for (int j = 0; j < blockSize; j++) {
                localAx[i] += localA[i * N + blockStart + j] * buffer[j];
            }
        }

        int nextOwner = (currentOwner - 1 + size) % size;
        int recvSize = counts[nextOwner];

        vector<double> recvBuf(recvSize);

        MPI_Sendrecv(
            buffer.data(),
            blockSize,
            MPI_DOUBLE,
            right,
            0,
            recvBuf.data(),
            recvSize,
            MPI_DOUBLE,
            left,
            0,
            comm,
            MPI_STATUS_IGNORE
        );

        buffer = recvBuf;
        blockSize = recvSize;
        currentOwner = nextOwner;
        blockStart = displs[currentOwner];
    }

    return localAx;
}

vector<double> simpleIterationMethodMPIv2(
    const vector<double>& localA,
    const vector<double>& localB,
    int N,
    int localRows,
    int rowStart,
    double tau,
    double epsilon,
    int maxIterations,
    MPI_Comm comm
) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    vector<int> counts, displs;
    buildCountsAndDispls(N, size, counts, displs);

    vector<double> xLocal(localRows, 0.0);
    vector<double> xLocalNew(localRows);

    double normB = vectorNorm(localB, comm);
    if (normB == 0.0) {
        normB = 1.0;
    }

    for (int iter = 0; iter < maxIterations; iter++) {

        vector<double> localAx = multiplyLocalMatrixVector(
            localA,
            xLocal,
            N,
            localRows,
            rowStart,
            counts,
            displs,
            comm
        );

        double localResidualSum = 0.0;

        for (int i = 0; i < localRows; i++) {
            double residual = localAx[i] - localB[i];
            localResidualSum += residual * residual;
            xLocalNew[i] = xLocal[i] - tau * residual;
        }

        double globalResidualSum;

        MPI_Allreduce(
            &localResidualSum,
            &globalResidualSum,
            1,
            MPI_DOUBLE,
            MPI_SUM,
            comm
        );

        double criterion = sqrt(globalResidualSum) / normB;

        if (criterion < epsilon) {
            if (rank == 0) {
                cout << "Converged in " << iter << " iterations\n";
            }
            xLocal = xLocalNew;
            break;
        }

        xLocal = xLocalNew;
    }

    return xLocal;
}

int main(int argc, char* argv[]) {

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

    vector<double> localB(localRows, N + 1);

    double startTime = MPI_Wtime();

    vector<double> xLocal = simpleIterationMethodMPIv2(
        localA,
        localB,
        N,
        localRows,
        rowStart,
        tau,
        epsilon,
        maxIterations,
        MPI_COMM_WORLD
    );

    double localMaxDiff = 0.0;
    for (double xi : xLocal) {
        localMaxDiff = max(localMaxDiff, abs(1.0 - xi));
    }
    double globalMaxDiff = 0.0;
    MPI_Allreduce(
        &localMaxDiff,
        &globalMaxDiff,
        1,
        MPI_DOUBLE,
        MPI_MAX,
        MPI_COMM_WORLD
    );

    double endTime = MPI_Wtime();

    if (rank == 0) {
        cout << fixed << setprecision(10);
        cout << "Elapsed time: " << endTime - startTime << " sec\n";
        cout << "max_diff = " << globalMaxDiff << "\n";
        cout << "good answer? " << ((globalMaxDiff < epsilon) ? "Yes" : "No") << "\n";
    }

    MPI_Finalize();

    return 0;
}