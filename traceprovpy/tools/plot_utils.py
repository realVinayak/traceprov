# Provides barebone utils for plots.
# Majority of the work is still done by respective benchmarks dirs.

# Every benchmark needs to be of this class (so that arguments can be captured.)
# This is done to improve reliability.
import argparse
from typing import Dict, NamedTuple, Tuple
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
        if len(self.values) < 10:
            return 0.0
        return statistics.median(self.values)

    def compute_stddev(self) -> float:
        if len(self.values) < 10:
            return 0.0
        stdev = statistics.stdev(self.values)
        print(self.label, stdev, self.values)

        return statistics.stdev(self.values)


class PlotMap:
    base = "base"
    traceprov = "traceprov"
    traceprov_infer = "traceprov (S + I)"
    traceprov_f_infer = "traceprov (F + S + I)"
    duckdb_time = "duckdb"
    traceprov_infer_legacy = "traceprov_legacy (S + I)"

    ticker = "$KEY"
    idx_first = False

    @staticmethod
    def annotate_with_key(in_str: str, key: Tuple[int, bool]):
        args = [str(key[0]).rjust(8, "0"), str(key[1])]
        if PlotMap.idx_first:
            args.reverse()
        return f"{PlotMap.ticker}{','.join(args)}-{in_str}"

    @staticmethod
    def strip_thread(in_str: str):
        assert PlotMap.ticker in in_str
        rest = in_str[len(PlotMap.ticker) :]
        thread_str = rest[0 : rest.index("-")].split(",")
        thread_count = int(thread_str[0])
        idx = eval(thread_str[1])
        return (rest[rest.index("-") + 1 :], (thread_count, idx))

    @staticmethod
    def is_key(in_str: str):
        return PlotMap.ticker in in_str

    @staticmethod
    def get_label(in_str: str):
        if not PlotMap.is_key(in_str):
            return in_str
        rest, (thread, is_idx) = PlotMap.strip_thread(in_str)
        return f"{rest} (thread: {thread}, index: {is_idx})"


def flatten_simple_base(results: list[dict]):
    timings = []
    for result in results:
        assert "explain_time" in result
        timings.append(result["explain_time"])
    return timings


def get_captured(in_result):
    return in_result["captured"][0][0]


def get_duckdb_results(query_result: Dict):
    for key, value in query_result.items():
        # print(key)
        if "DUCKDB_INFERENCE" in key:
            return value
    assert False, "Should always find it!"


def apply_func(query_result, func):
    return {
        qnum: {cat: func(cat_result) for (cat, cat_result) in qresult.items()}
        for qnum, qresult in query_result.items()
    }


def extract_threads(thread_sym: str, dir_name: str):
    if thread_sym in dir_name:
        terminal = dir_name.index(thread_sym)
        thread_count = dir_name[terminal:].replace(f"{thread_sym}_", "").split("_")[0]
        return thread_count
    return None


def extract_idx(dir_name: str):
    if "no_index" in dir_name:
        return False
    if "idx" in dir_name:
        return True
    if "index" in dir_name:
        return True
    return None


def guard_no_len(func):
    def _func(in_list: list):
        if len(in_list) == 0:
            return 0
        return func(in_list)

    return _func


def extract_duckdb_timings(query_result: Dict):
    duckdb_results = get_duckdb_results(query_result)
    mapped = [
        [(row["time"] / (1_000_000)) for row in result["duckdb_inference"]]
        for result in duckdb_results
    ]
    total_across_layers = [sum(layer_time) for layer_time in zip(*mapped, strict=True)]
    return total_across_layers


def assert_is_list(in_value):
    assert isinstance(in_value, list)
    return in_value


def flipper(previous: dict, current: Tuple[int, dict]):
    qnum, qresult = current
    return {
        **previous,
        **{
            cat_key: {**previous.get(cat_key, {}), qnum: cat_result}
            for (cat_key, cat_result) in qresult.items()
        },
    }


CAPTURE_ORDER = [PlotMap.base, PlotMap.traceprov]

INFER_ORDER = [PlotMap.traceprov_infer, PlotMap.traceprov_infer_legacy]


def is_infer(x):
    return x in INFER_ORDER or PlotMap.is_key(x)


def compute_slowdown(first, second):
    return [((f / s) - 1) * 100 for f, s in zip(first, second)]
