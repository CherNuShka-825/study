from math import sqrt


def f(x, a, b, c):
    return x**3 + a * x**2 + b * x + c


def bisection(left, right, eps, a, b, c):
    f_left = f(left, a, b, c)

    while True:
        mid = (left + right) / 2
        f_mid = f(mid, a, b, c)

        if abs(f_mid) < eps:
            return mid

        if f_left * f_mid < 0:
            right = mid
        else:
            left = mid
            f_left = f_mid


def find_right_interval(start, delta, a, b, c):
    left = start
    right = start + delta

    while f(left, a, b, c) * f(right, a, b, c) > 0:
        left = right
        right += delta

    return left, right


def find_left_interval(start, delta, a, b, c):
    right = start
    left = start - delta

    while f(left, a, b, c) * f(right, a, b, c) > 0:
        right = left
        left -= delta

    return left, right


def solve_cubic(eps, delta, a, b, c):
    roots = []

    # Производная:
    # f'(x) = 3x^2 + 2ax + b
    #
    # Дискриминант производной:
    # D = (2a)^2 - 4 * 3 * b
    derivative_d = 4 * a**2 - 12 * b

    # Случай 1: производная не имеет двух различных корней.
    # Кубическая функция монотонна -> корень один.
    if derivative_d <= 0:
        f0 = f(0, a, b, c)

        if abs(f0) < eps:
            roots.append(0.0)

        elif f0 < -eps:
            left, right = find_right_interval(0, delta, a, b, c)
            roots.append(bisection(left, right, eps, a, b, c))

        else:
            left, right = find_left_interval(0, delta, a, b, c)
            roots.append(bisection(left, right, eps, a, b, c))

        return roots

    # Случай 2: у производной есть два различных корня.
    sqrt_d = sqrt(derivative_d)

    alpha = (-2 * a - sqrt_d) / 6
    beta = (-2 * a + sqrt_d) / 6

    f_alpha = f(alpha, a, b, c)
    f_beta = f(beta, a, b, c)

    # а) оба значения положительные
    if f_alpha > eps and f_beta > eps:
        left, right = find_left_interval(alpha, delta, a, b, c)

        roots.append(bisection(left, right, eps, a, b, c))

    # б) оба отрицательные
    elif f_alpha < -eps and f_beta < -eps:
        left, right = find_right_interval(beta, delta, a, b, c)

        roots.append(bisection(left, right, eps, a, b, c))

    # в) beta является двойным корнем
    elif f_alpha > eps and abs(f_beta) < eps:
        left, right = find_left_interval(alpha, delta, a, b, c)

        roots.append(bisection(left, right, eps, a, b, c))

        roots.append(beta)

    # г) alpha является двойным корнем
    elif abs(f_alpha) < eps and f_beta < -eps:
        roots.append(alpha)

        left, right = find_right_interval(beta, delta, a, b, c)

        roots.append(bisection(left, right, eps, a, b, c))

    # д) три корня
    elif f_alpha > eps and f_beta < -eps:
        # Первый: (-inf, alpha)
        left, right = find_left_interval(alpha, delta, a, b, c)
        root1 = bisection(left, right, eps, a, b, c)

        # Второй: (alpha, beta)
        root2 = bisection(alpha, beta, eps, a, b, c)

        # Третий: (beta, +inf)
        left, right = find_right_interval(beta, delta, a, b, c)
        root3 = bisection(left, right, eps, a, b, c)

        roots.extend([root1, root2, root3])

    # е) особый случай из условия
    elif abs(f_alpha) < eps and abs(f_beta) < eps:
        roots.append((alpha + beta) / 2)

    return roots


eps, delta, a, b, c = map(float, input("Введите eps, delta, a, b, c: ").split())

roots = solve_cubic(eps, delta, a, b, c)

print()

if len(roots) == 1:
    print("У данного кубического уравнения один корень:")
elif len(roots) == 2:
    print("У данного кубического уравнения два корня:")
else:
    print("У данного кубического уравнения три корня:")

for i, root in enumerate(roots, start=1):
    print(f"{i}) x{i} = {root}, f(x{i}) = {f(root, a, b, c)}")
