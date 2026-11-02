import math


def f(x):
    return math.exp(x)


a = 1.0
b = 2.0
exact = math.exp(2) - math.exp(1)


def left_rectangles(n):
    h = (b - a) / n
    return h * sum(f(a + i * h) for i in range(n))


def trapezoids(n):
    h = (b - a) / n
    return h * ((f(a) + f(b)) / 2 + sum(f(a + i * h) for i in range(1, n)))


def simpson(n):
    if n % 2 != 0:
        raise ValueError("n должно быть чётным")

    h = (b - a) / n

    odd_sum = sum(f(a + i * h) for i in range(1, n, 2))
    even_sum = sum(f(a + i * h) for i in range(2, n, 2))

    return h / 3 * (f(a) + f(b) + 4 * odd_sum + 2 * even_sum)


def runge_order(method, n):
    s_n = method(n)
    s_2n = method(2 * n)
    s_4n = method(4 * n)

    numerator = s_n - s_2n
    denominator = s_2n - s_4n

    if numerator == 0 or denominator == 0:
        return float("nan")

    return math.log2(abs(numerator / denominator))


methods = [
    ("Левые прямоугольники", left_rectangles),
    ("Трапеции", trapezoids),
    ("Симпсон", simpson),
]

print(f"Точное значение: {exact:.12f}")

for name, method in methods:
    print(f"\nМетод: {name}")
    print(f"{'n':>6} {'S_n':>18} {'Ошибка':>18} {'k':>12}")

    for n in [10, 20, 40, 100, 200, 400, 1000, 2000, 4000]:
        s_n = method(n)
        error = abs(s_n - exact)
        k = runge_order(method, n)

        print(f"{n:6d} {s_n:18.12f} {error:18.10e} {k:12.6f}")
