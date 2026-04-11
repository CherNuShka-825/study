#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <omp.h>

using namespace std;

void fillMat(vector<double>& A, int N) {
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i == j) {
                A[i * N + j] = 2.0;
            } else {
                A[i * N + j] = 1.0;
            }
        }
    }
}

vector<double> simpleIterationMethodOMPv1(
    const vector<double>& A,
    const vector<double>& b,
    int N,
    double tau,
    double epsilon,
    int maxIterations
) {
    vector<double> x(N, 0.0);
    vector<double> xNew(N, 0.0);

    bool converged = false;
    int iterationsDone = 0;

    double normB2 = 0.0;
    double normB;
    double residualSum = 0.0;

    #pragma omp parallel for reduction(+:normB2) schedule(runtime)
    for (int i = 0; i < N; ++i) {
        normB2 += b[i] * b[i];
    }

    if (normB2 == 0.0) {
        normB = 1.0;
    } else {
        normB = sqrt(normB2);
    }

    for (int iter = 0; iter < maxIterations && !converged; ++iter) {
        residualSum = 0.0;

        #pragma omp parallel for reduction(+:residualSum) schedule(runtime)
        for (int i = 0; i < N; ++i) {
            double Ax_i = 0.0;
            for (int j = 0; j < N; ++j) {
                Ax_i += A[i * N + j] * x[j];
            }

            double residual = Ax_i - b[i];
            residualSum += residual * residual;
            xNew[i] = x[i] - tau * residual;
        }

        double criterion = sqrt(residualSum) / normB;
        iterationsDone = iter + 1;
        converged = (criterion < epsilon);

        #pragma omp parallel for schedule(runtime)
        for (int i = 0; i < N; ++i) {
            x[i] = xNew[i];
        }
    }

    if (converged) {
        cout << "Converged in " << iterationsDone << " iterations.\n";
    } else {
        cout << "Maximum number of iterations reached.\n";
    }

    return x;
}

int main() {
    int N = 19000;
    double tau = 0.0001;
    double epsilon = 1e-5;
    int maxIterations = 1000000;

    vector<double> A(N * N);
    fillMat(A, N);

    vector<double> b(N, static_cast<double>(N + 1));

    double startTime = omp_get_wtime();

    vector<double> x = simpleIterationMethodOMPv1(
        A,
        b,
        N,
        tau,
        epsilon,
        maxIterations
    );

    double endTime = omp_get_wtime();

    double max_diff = 0.0;

    #pragma omp parallel for reduction(max:max_diff) schedule(runtime)
    for (int i = 0; i < N; ++i) {
        double diff = fabs(1.0 - x[i]);
        if (diff > max_diff) {
            max_diff = diff;
        }
    }

    cout << fixed << setprecision(10);
    cout << "Elapsed time: " << (endTime - startTime) << " sec\n";
    cout << "max_diff = " << max_diff << "\n";
    cout << "good answer? " << ((max_diff < epsilon) ? "Yes" : "No") << "\n";

    return 0;
}