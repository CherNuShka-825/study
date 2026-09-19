def progonka(a, c, b, f):
    n = len(f)

    alpha = [0] * n
    beta = [0] * n
    x = [0] * n

    alpha[0] = b[0] / c[0]
    beta[0] = f[0] / c[0]

    for i in range(1, n):
        d = c[i] - a[i] * alpha[i - 1]

        if i < n - 1:
            alpha[i] = b[i] / d

        beta[i] = (f[i] + a[i] * beta[i - 1]) / d

    x[n - 1] = beta[n - 1]

    for i in range(n - 2, -1, -1):
        x[i] = alpha[i] * x[i + 1] + beta[i]

    return x


a = [0, -1, -2, 3]
b = [-1, -1, 2, 0]
c = [7, 7, 7, 7]
f = [6, -5, 7, -10]
x = progonka(a, c, b, f)
print(x)

a = [0, -2, -3, -4]
b = [-1, 3, 2, 0]
c = [8, 8, 8, 8]
f = [13.5, 11.6, 24.3, 45.6]
x = progonka(a, c, b, f)
print(x)
