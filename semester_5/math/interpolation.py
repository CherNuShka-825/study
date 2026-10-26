#!/usr/bin/env python3
"""Сравнение интерполяции |x| полиномом Лагранжа и натуральным кубическим сплайном.

Примеры:
    python interpolation.py 5
    python interpolation.py                  # интерактивный ввод n
    python interpolation.py 7 --no-show
    python interpolation.py 3 --output n3.png

Зависимости: numpy, matplotlib.
"""

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.lines import Line2D


def lagrange(x_nodes: np.ndarray, y_nodes: np.ndarray, x: np.ndarray) -> np.ndarray:
    """Вычислить интерполяционный многочлен Лагранжа (барицентрическая форма)."""
    n = len(x_nodes)
    weights = np.empty(n, dtype=float)
    for i in range(n):
        weights[i] = 1.0 / np.prod(x_nodes[i] - np.delete(x_nodes, i))
    weights /= np.max(np.abs(weights))

    distances = x[:, None] - x_nodes[None, :]
    matches = np.isclose(distances, 0.0, atol=1e-13, rtol=0)
    with np.errstate(divide="ignore", invalid="ignore"):
        ratios = weights[None, :] / distances
        result = (ratios @ y_nodes) / np.sum(ratios, axis=1)

    match_rows = np.any(matches, axis=1)
    if np.any(match_rows):
        result[match_rows] = y_nodes[np.argmax(matches[match_rows], axis=1)]
    return result


def natural_cubic_spline(x: np.ndarray, y: np.ndarray):
    """Коэффициенты a,b,c,d в p_i(t)=a_i+b_i*t+c_i*t²/2+d_i*t³/6.

    Естественные граничные условия: p''(-1)=p''(1)=0.
    """
    n = len(x)
    h = np.diff(x)
    system = np.zeros((n, n))
    rhs = np.zeros(n)
    system[0, 0] = 1.0
    system[-1, -1] = 1.0

    for i in range(1, n - 1):
        system[i, i - 1] = h[i - 1]
        system[i, i] = 2 * (h[i - 1] + h[i])
        system[i, i + 1] = h[i]
        rhs[i] = 6 * ((y[i + 1] - y[i]) / h[i] - (y[i] - y[i - 1]) / h[i - 1])

    c = np.linalg.solve(system, rhs)  # c_i = p''(x_i), включая c_0=c_{n-1}=0
    a = y[:-1].copy()
    b = (y[1:] - y[:-1]) / h - h * (2 * c[:-1] + c[1:]) / 6
    d = (c[1:] - c[:-1]) / h
    return a, b, c, d


def eval_spline_piece(
    x: np.ndarray, left: float, a: float, b: float, c: float, d: float
) -> np.ndarray:
    t = x - left
    return a + b * t + c * t**2 / 2 + d * t**3 / 6


def spline_on_grid(nodes: np.ndarray, xx: np.ndarray, coeffs) -> np.ndarray:
    a, b, c, d = coeffs
    inds = np.searchsorted(nodes, xx, side="right") - 1
    inds = np.clip(inds, 0, len(nodes) - 2)
    t = xx - nodes[inds]
    return a[inds] + b[inds] * t + c[inds] * t**2 / 2 + d[inds] * t**3 / 6


def make_figure(n: int, output: Path, show: bool):
    nodes = np.linspace(-1, 1, n)
    values = np.abs(nodes)
    xx = np.linspace(-1, 1, 3001)
    exact = np.abs(xx)
    poly = lagrange(nodes, values, xx)
    coeffs = natural_cubic_spline(nodes, values)
    spline = spline_on_grid(nodes, xx, coeffs)

    err_poly = np.max(np.abs(poly - exact))
    err_spline = np.max(np.abs(spline - exact))

    plt.rcParams.update(
        {
            "font.family": "DejaVu Sans",
            "font.size": 10,
            "axes.spines.top": False,
            "axes.spines.right": False,
            "axes.titlesize": 12,
            "axes.titleweight": "bold",
            "axes.labelsize": 11,
        }
    )
    fig, axes = plt.subplots(1, 2, figsize=(14, 5.4), sharex=True, sharey=True)
    fig.patch.set_facecolor("#f4f6fb")
    fig.suptitle(
        f"Интерполяция функции  f(x) = |x|     •     n = {n} узлов",
        fontsize=16,
        fontweight="bold",
        y=0.98,
        color="#202637",
    )
    base_color = "#303849"
    lag_color = "#e06c54"

    for ax in axes:
        ax.set_facecolor("#ffffff")
        ax.grid(alpha=0.22, color="#9caac0", linewidth=0.75)
        ax.set_axisbelow(True)
        ax.plot(
            xx,
            exact,
            color=base_color,
            linestyle="--",
            linewidth=2,
            label="Исходная |x|",
            zorder=3,
        )
        ax.scatter(
            nodes,
            values,
            s=32,
            facecolor="white",
            edgecolor=base_color,
            linewidth=1.8,
            zorder=7,
            label="Узлы",
        )
        ax.set_xlabel("x")
        ax.set_xlim(-1.05, 1.05)
        ax.set_xticks(np.linspace(-1, 1, 5))
        ax.tick_params(colors="#444c5b")
        ax.spines["left"].set_color("#aab2c0")
        ax.spines["bottom"].set_color("#aab2c0")
    axes[0].set_ylabel("y")

    axes[0].plot(xx, poly, color=lag_color, linewidth=2.5, label="Лагранж", zorder=4)
    axes[0].fill_between(
        xx,
        poly,
        exact,
        color=lag_color,
        alpha=0.15,
        label="Отклонение от |x|",
        zorder=1,
    )
    axes[0].set_title(f"Полином Лагранжа   ·   max ошибка = {err_poly:.4g}", pad=14)
    axes[0].legend(
        loc="upper center",
        bbox_to_anchor=(0.5, -0.19),
        ncol=3,
        frameon=False,
        fontsize=9,
    )

    a, b, c, d = coeffs
    colors = plt.get_cmap("turbo")(np.linspace(0.08, 0.86, n - 1))
    for i in range(n - 1):
        section = np.linspace(nodes[i], nodes[i + 1], 150)
        yy = eval_spline_piece(section, nodes[i], a[i], b[i], c[i], d[i])
        axes[1].plot(section, yy, color=colors[i], linewidth=3, zorder=5)
        if n <= 11:
            middle = 0.5 * (nodes[i] + nodes[i + 1])
            midy = eval_spline_piece(
                np.array([middle]), nodes[i], a[i], b[i], c[i], d[i]
            )[0]
            axes[1].annotate(
                f"p{i + 1}",
                (middle, midy),
                xytext=(0, 11),
                textcoords="offset points",
                ha="center",
                va="bottom",
                fontsize=9,
                fontweight="bold",
                color=colors[i],
            )

    axes[1].set_title(f"кубический сплайн   ·   max ошибка = {err_spline:.4g}", pad=14)
    legend_handles = [
        Line2D([0], [0], color=base_color, lw=2, linestyle="--", label="Исходная |x|"),
        Line2D([0], [0], color="#2586b0", lw=3, label=f"{n - 1} сегментов сплайна"),
        Line2D(
            [0],
            [0],
            color=base_color,
            marker="o",
            markersize=6,
            markerfacecolor="white",
            lw=0,
            label="Узлы",
        ),
    ]
    axes[1].legend(
        handles=legend_handles,
        loc="upper center",
        bbox_to_anchor=(0.5, -0.19),
        ncol=3,
        frameon=False,
        fontsize=9,
    )

    low = min(-0.06, float(np.min(poly)) - 0.08, float(np.min(spline)) - 0.08)
    high = max(1.12, float(np.max(poly)) + 0.1, float(np.max(spline)) + 0.1)
    axes[0].set_ylim(low, high)
    fig.subplots_adjust(left=0.07, right=0.985, bottom=0.19, top=0.84, wspace=0.14)
    output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output, dpi=180, facecolor=fig.get_facecolor())

    print(f"Узлы x_i: {np.array2string(nodes, precision=5)}")
    print(f"Значения f_i: {np.array2string(values, precision=5)}")
    print(f"Максимальная ошибка Лагранжа на сетке: {err_poly:.8g}")
    print(f"Максимальная ошибка сплайна на сетке: {err_spline:.8g}")
    print("\nКоэффициенты сплайна p_i(x) = a_i + b_i*t + c_i*t²/2 + d_i*t³/6, t=x-x_i:")
    for i in range(n - 1):
        print(
            f"p{i + 1} на [{nodes[i]:.6g}, {nodes[i + 1]:.6g}]: "
            f"a={a[i]:+.6g}; b={b[i]:+.6g}; c={c[i]:+.6g}; d={d[i]:+.6g}"
        )
    print(f"\nГрафик сохранён: {output.resolve()}")

    if show:
        plt.show()
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(
        description="Графики Лагранжа и естественного кубического сплайна для |x| на [-1,1]."
    )
    parser.add_argument(
        "n", type=int, nargs="?", help="число узлов (например, 3, 5, 7)"
    )
    parser.add_argument("--output", "-o", type=Path, help="имя PNG-файла")
    parser.add_argument(
        "--no-show", action="store_true", help="сохранить без открытия окна"
    )
    args = parser.parse_args()
    n = args.n
    if n is None:
        try:
            n = int(input("Введите количество узлов n (например, 3, 5 или 7): "))
        except ValueError:
            parser.error("n должно быть целым числом")
    if n < 3 or n > 50:
        parser.error("выберите число узлов от 3 до 50")
    output = args.output or Path(f"interpolation_n{n}.png")
    make_figure(n, output, show=not args.no_show)


if __name__ == "__main__":
    main()
