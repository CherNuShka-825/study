#include <mpi.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cmath>
#include <algorithm>

static inline int idx(int i, int j, int cols) {
    return i * cols + j;
}

void fillOnes(std::vector<double>& M) {
    std::fill(M.begin(), M.end(), 1.0);
}

void createGrid(MPI_Comm& gridComm, MPI_Comm& rowComm, MPI_Comm& colComm,
                int& rank, int& size,
                int& p1, int& p2,
                int& x, int& y) {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int dims[2] = {0, 0};
    MPI_Dims_create(size, 2, dims);

    int periods[2] = {0, 0};
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &gridComm);

    int coords[2];
    MPI_Cart_coords(gridComm, rank, 2, coords);

    p1 = dims[0];
    p2 = dims[1];
    x = coords[0];
    y = coords[1];

    MPI_Comm_split(gridComm, x, y, &rowComm);
    MPI_Comm_split(gridComm, y, x, &colComm);
}

bool checkSizes(int N, int p1, int p2, int rank) {
    if (N % p1 != 0 || N % p2 != 0) {
        if (rank == 0) {
            std::cerr << "Error: N must be divisible by p1 and p2\n";
        }
        return false;
    }
    return true;
}

void scatterA(const std::vector<double>& A, std::vector<double>& ARow,
              int N, int localRows,
              MPI_Comm gridComm, int x, int y) {
    MPI_Comm col0Comm = MPI_COMM_NULL;

    if (y == 0) {
        MPI_Comm_split(gridComm, 0, x, &col0Comm);

        const double* sendBuf = A.empty() ? nullptr : A.data();

        MPI_Scatter(sendBuf,
                    localRows * N,
                    MPI_DOUBLE,
                    ARow.data(),
                    localRows * N,
                    MPI_DOUBLE,
                    0,
                    col0Comm);

        MPI_Comm_free(&col0Comm);
    }
}

void scatterB(const std::vector<double>& B, std::vector<double>& BCol,
              int N, int localCols,
              MPI_Comm gridComm, int x, int y) {
    MPI_Comm row0Comm = MPI_COMM_NULL;

    if (x == 0) {
        MPI_Comm_split(gridComm, 0, y, &row0Comm);

        MPI_Datatype stripeType;
        MPI_Type_vector(N,
                        localCols,
                        N,
                        MPI_DOUBLE,
                        &stripeType);

        MPI_Datatype resizedStripeType;
        MPI_Type_create_resized(stripeType,
                                0,
                                static_cast<MPI_Aint>(localCols) * sizeof(double),
                                &resizedStripeType);
        MPI_Type_commit(&resizedStripeType);

        const double* sendBuf = B.empty() ? nullptr : B.data();

        MPI_Scatter(sendBuf,
                    1,
                    resizedStripeType,
                    BCol.data(),
                    N * localCols,
                    MPI_DOUBLE,
                    0,
                    row0Comm);

        MPI_Type_free(&resizedStripeType);
        MPI_Type_free(&stripeType);
        MPI_Comm_free(&row0Comm);
    }
}

void broadcastStripes(std::vector<double>& Arow, std::vector<double>& Bcol,
                      int localRows, int localCols, int N,
                      MPI_Comm rowComm, MPI_Comm colComm) {
    MPI_Bcast(Arow.data(), localRows * N, MPI_DOUBLE, 0, rowComm);
    MPI_Bcast(Bcol.data(), N * localCols, MPI_DOUBLE, 0, colComm);
}

void multiplyLocal(const std::vector<double>& Arow,
                   const std::vector<double>& Bcol,
                   std::vector<double>& Cloc,
                   int localRows, int localCols, int N) {
    for (int i = 0; i < localRows; ++i) {
        for (int k = 0; k < N; ++k) {
            double a = Arow[idx(i, k, N)];
            const double* brow = &Bcol[idx(k, 0, localCols)];
            double* crow = &Cloc[idx(i, 0, localCols)];

            for (int j = 0; j < localCols; ++j) {
                crow[j] += a * brow[j];
            }
        }
    }
}

void placeBlock(std::vector<double>& C, const std::vector<double>& block,
                int blockRow, int blockCol,
                int localRows, int localCols, int N) {
    for (int i = 0; i < localRows; ++i) {
        for (int j = 0; j < localCols; ++j) {
            C[idx(blockRow * localRows + i, blockCol * localCols + j, N)] =
                block[idx(i, j, localCols)];
        }
    }
}

void gatherC(std::vector<double>& C, const std::vector<double>& Cloc,
             int localRows, int localCols, int N,
             int rank, int p1, int p2, MPI_Comm gridComm) {
    if (rank == 0) {
        placeBlock(C, Cloc, 0, 0, localRows, localCols, N);

        std::vector<double> tmp(localRows * localCols);

        for (int rx = 0; rx < p1; ++rx) {
            for (int cy = 0; cy < p2; ++cy) {
                int srcCoords[2] = {rx, cy};
                int srcRank;
                MPI_Cart_rank(gridComm, srcCoords, &srcRank);

                if (srcRank == 0) {
                    continue;
                }

                MPI_Recv(tmp.data(), localRows * localCols, MPI_DOUBLE, srcRank, 300,
                         gridComm, MPI_STATUS_IGNORE);

                placeBlock(C, tmp, rx, cy, localRows, localCols, N);
            }
        }
    } else {
        MPI_Send(Cloc.data(), localRows * localCols, MPI_DOUBLE, 0, 300, gridComm);
    }
}

bool checkResult(const std::vector<double>& C, int N) {
    double expected = static_cast<double>(N);

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (std::abs(C[idx(i, j, N)] - expected) > 1e-9) {
                return false;
            }
        }
    }
    return true;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int N = 1024;
    if (argc >= 2) {
        N = std::atoi(argv[1]);
    }

    MPI_Comm gridComm, rowComm, colComm;
    int rank, size, p1, p2, x, y;

    createGrid(gridComm, rowComm, colComm, rank, size, p1, p2, x, y);

    if (!checkSizes(N, p1, p2, rank)) {
        MPI_Comm_free(&rowComm);
        MPI_Comm_free(&colComm);
        MPI_Comm_free(&gridComm);
        MPI_Finalize();
        return 1;
    }

    int localRows = N / p1;
    int localCols = N / p2;

    std::vector<double> A;
    std::vector<double> B;
    std::vector<double> C;

    if (rank == 0) {
        A.resize(N * N);
        B.resize(N * N);
        C.resize(N * N);

        fillOnes(A);
        fillOnes(B);
    }

    std::vector<double> ARow(localRows * N);
    std::vector<double> BCol(N * localCols);
    std::vector<double> Cloc(localRows * localCols, 0.0);

    scatterA(A, ARow, N, localRows, gridComm, x, y);
    scatterB(B, BCol, N, localCols, gridComm, x, y);

    broadcastStripes(ARow, BCol, localRows, localCols, N, rowComm, colComm);

    MPI_Barrier(gridComm);
    double t0 = MPI_Wtime();

    multiplyLocal(ARow, BCol, Cloc, localRows, localCols, N);

    MPI_Barrier(gridComm);
    double t1 = MPI_Wtime();

    double localTime = t1 - t0;
    double maxTime = 0.0;
    MPI_Reduce(&localTime, &maxTime, 1, MPI_DOUBLE, MPI_MAX, 0, gridComm);

    gatherC(C, Cloc, localRows, localCols, N, rank, p1, p2, gridComm);

    if (rank == 0) {
        bool ok = checkResult(C, N);

        std::cout << "Grid: " << p1 << "x" << p2 << "\n";
        std::cout << "Matrix size: " << N << "x" << N << "\n";
        std::cout << "Time: " << std::fixed << std::setprecision(6)
                  << maxTime << " sec\n";
        std::cout << "Check: " << (ok ? "OK" : "FAILED") << "\n";
    }

    MPI_Comm_free(&rowComm);
    MPI_Comm_free(&colComm);
    MPI_Comm_free(&gridComm);

    MPI_Finalize();
    return 0;
}