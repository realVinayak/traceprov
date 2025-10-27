# Provides barebone utils for plots.
# Majority of the work is still done by respective benchmarks dirs.

# Every benchmark needs to be of this class (so that arguments can be captured.)
# This is done to improve reliability.
import argparse
from typing import NamedTuple
import statistics


class BenchmarkPlot:

    colors = [
        "tab:blue",
        "tab:orange",
        "tab:green",
        "tab:red",
        "tab:purple",
        "tab:brown",
        "tab:pink",
        "tab:gray",
        "tab:olive",
        "tab:cyan",
        "m",
        "k",
    ]

    def __init__(self, name: str):
        self.name = name
        self.plot_args = None
        # Makes a simple parser.
        # The caller can add more arguments if needed.
        parser = argparse.ArgumentParser(prog=f"result_analyzer_{self.name}")
        parser.add_argument("-r", "--root", required=True, type=str)
        parser.add_argument("-f", "--files", required=True, type=str, nargs="+")
        self.parser = parser

    def plot(self, **kwargs):
        self.plot_args = kwargs


class Plotable(NamedTuple):
    label: str
    values: list[float]

    def compute_median(self) -> float:
        if len(self.values) == 0:
            return 0.0
        return statistics.median(self.values)

    def compute_stddev(self) -> float:
        if len(self.values) == 0:
            return 0.0
        stdev = statistics.stdev(self.values)
        print(self.label, stdev, self.values)

        return statistics.stdev(self.values)
