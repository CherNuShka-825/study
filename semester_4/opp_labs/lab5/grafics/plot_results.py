#!/usr/bin/env python3

import argparse
import os
import re
from dataclasses import dataclass
from typing import Dict, List

import matplotlib.pyplot as plt


@dataclass
class Result:
    balance: str
    scenario: str
    processes: int
    time: float
    avg_lif: float


RESULT_RE = re.compile(
    r"RESULT\s+"
    r".*?balance=(?P<balance>\S+)\s+"
    r".*?scenario=(?P<scenario>\S+)\s+"
    r".*?processes=(?P<processes>\d+)\s+"
    r".*?time=(?P<time>[0-9.eE+-]+)\s+"
    r".*?avg_lif=(?P<avg_lif>[0-9.eE+-]+)"
)


def parse_file(path: str) -> Dict[int, Result]:
    results: Dict[int, Result] = {}

    with open(path, "r", encoding="utf-8") as file:
        for line in file:
            match = RESULT_RE.search(line)
            if not match:
                continue

            result = Result(
                balance=match.group("balance"),
                scenario=match.group("scenario"),
                processes=int(match.group("processes")),
                time=float(match.group("time")),
                avg_lif=float(match.group("avg_lif")),
            )

            # Если в файле несколько RESULT для одного np,
            # оставляем последний найденный.
            results[result.processes] = result

    if not results:
        raise RuntimeError(f"No RESULT lines found in {path}")

    return results


def sorted_processes(off: Dict[int, Result], on: Dict[int, Result]) -> List[int]:
    common = sorted(set(off.keys()) & set(on.keys()))

    if not common:
        raise RuntimeError("No common process counts between files")

    return common


def values_time(results: Dict[int, Result], processes: List[int]) -> List[float]:
    return [results[p].time for p in processes]


def values_lif(results: Dict[int, Result], processes: List[int]) -> List[float]:
    return [results[p].avg_lif for p in processes]


def values_speedup(results: Dict[int, Result], processes: List[int]) -> List[float]:
    if 1 not in results:
        raise RuntimeError("Cannot compute speedup: np=1 result is missing")

    t1 = results[1].time

    return [t1 / results[p].time for p in processes]


def values_efficiency(results: Dict[int, Result], processes: List[int]) -> List[float]:
    speedups = values_speedup(results, processes)

    return [
        speedup / p
        for speedup, p in zip(speedups, processes)
    ]


def plot_two_series(
    processes: List[int],
    off_values: List[float],
    on_values: List[float],
    title: str,
    y_label: str,
    output_path: str,
) -> None:
    plt.figure(figsize=(9, 6))

    plt.plot(processes, off_values, marker="o", label="balance off")
    plt.plot(processes, on_values, marker="o", label="balance on")

    plt.title(title)
    plt.xlabel("Number of MPI processes")
    plt.ylabel(y_label)

    plt.xticks(processes)
    plt.ylim(bottom=0)

    plt.grid(True)
    plt.legend()
    plt.tight_layout()

    plt.savefig(output_path, dpi=200)
    plt.close()


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Build performance graphs from lab5 output files."
    )

    parser.add_argument(
        "off_file",
        help="File with balance=off output"
    )

    parser.add_argument(
        "on_file",
        help="File with balance=on output"
    )

    parser.add_argument(
        "--out-dir",
        default="graphs",
        help="Directory for generated graphs"
    )

    parser.add_argument(
        "--title",
        default=None,
        help="Algorithm/scenario name for graph titles"
    )

    args = parser.parse_args()

    off = parse_file(args.off_file)
    on = parse_file(args.on_file)

    processes = sorted_processes(off, on)

    scenario = args.title
    if scenario is None:
        scenario = on[processes[0]].scenario

    os.makedirs(args.out_dir, exist_ok=True)

    off_time = values_time(off, processes)
    on_time = values_time(on, processes)

    off_speedup = values_speedup(off, processes)
    on_speedup = values_speedup(on, processes)

    off_efficiency = values_efficiency(off, processes)
    on_efficiency = values_efficiency(on, processes)

    off_lif = values_lif(off, processes)
    on_lif = values_lif(on, processes)

    plot_two_series(
        processes,
        off_time,
        on_time,
        f"{scenario}: program execution time",
        "Time, seconds",
        os.path.join(args.out_dir, "time.png"),
    )

    plot_two_series(
        processes,
        off_speedup,
        on_speedup,
        f"{scenario}: speedup",
        "Speedup",
        os.path.join(args.out_dir, "speedup.png"),
    )

    plot_two_series(
        processes,
        off_efficiency,
        on_efficiency,
        f"{scenario}: efficiency",
        "Efficiency",
        os.path.join(args.out_dir, "efficiency.png"),
    )

    plot_two_series(
        processes,
        off_lif,
        on_lif,
        f"{scenario}: load imbalance factor",
        "Average LIF",
        os.path.join(args.out_dir, "lif.png"),
    )

    print("Graphs saved to:", args.out_dir)
    print("Processes:", processes)
    print("Generated:")
    print("  time.png")
    print("  speedup.png")
    print("  efficiency.png")
    print("  lif.png")


if __name__ == "__main__":
    main()
