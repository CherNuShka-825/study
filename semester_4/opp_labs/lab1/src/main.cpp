#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

using namespace std;

double vectorNorm(const vector<double>& v) {
    double sum = 0.0;
    for (double value : v) {
        sum += value * value;
    }
    return sqrt(sum);
}

vector<double> multiplyMatrixVector(const vector<double>& A, const vector<double>& x, int N) {
    vector<double> result(N, 0.0);

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            result[i] += A[i * N + j] * x[j];
        }
    }

    return result;
}

vector<double> subtractVectors(const vector<double>& a, const vector<double>& b) {
    int N = static_cast<int>(a.size());
    vector<double> result(N);

    for (int i = 0; i < N; ++i) {
        result[i] = a[i] - b[i];
    }

    return result;
}

vector<double> simpleIterationMethod(
    const vector<double>& A,
    const vector<double>& b,
    int N,
    double tau,
    double epsilon,
    int maxIterations
) {
    vector<double> x(N, 0.0);

    double normB = vectorNorm(b);
    if (normB == 0.0) {
        normB = 1.0;
    }

    for (int iter = 0; iter < maxIterations; ++iter) {
        vector<double> Ax = multiplyMatrixVector(A, x, N);
        vector<double> residual = subtractVectors(Ax, b);

        double criterion = vectorNorm(residual) / normB;

        if (criterion < epsilon) {
            cout << "Converged in " << iter << " iterations.\n";
            cout << "Final residual: " << criterion << "\n";
            return x;
        }

        vector<double> xNew(N);
        for (int i = 0; i < N; ++i) {
            xNew[i] = x[i] - tau * residual[i];
        }

        x = xNew;
    }

    cout << "Maximum number of iterations reached.\n";
    return x;
}

void fillMat(vector<double>& A, int N) {
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

int main() {
    int N = 14000;
    double tau = 0.0001;
    double epsilon = 1e-5;
    int maxIterations = 1000000;

    vector<double> A(N * N);
    fillMat(A, N);

    vector<double> b(N, static_cast<double>(N + 1));

    vector<double> x = simpleIterationMethod(A, b, N, tau, epsilon, maxIterations);

    cout << fixed << setprecision(10);
    cout << "Solution:\n";
    // for (int i = 0; i < N; ++i) {
    //     cout << "x[" << i << "] = " << x[i] << "\n";
    // }
    cout << "x[" << 0 << "] = " << x[0] << "\n";

    return 0;
}