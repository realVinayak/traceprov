# Provides barebone utils for plots.
# Majority of the work is still done by respective benchmarks dirs.

# Every benchmark needs to be of this class (so that arguments can be captured.)
# This is done to improve reliability.
import argparse
from datetime import datetime
import glob
import math
import os
from pathlib import Path
import sys
from typing import Any, Dict, NamedTuple, Tuple
import statistics

from matplotlib import ticker

from traceprovpy.tools.file_utils import json_read_file, just_read, just_write
import json
from matplotlib.transforms import blended_transform_factory


class MaskedParser(argparse.ArgumentParser):
    _other: "BenchmarkPlot"

    def __init__(self, other: "BenchmarkPlot", *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._other = other

    def parse_args(self, *args, **kwargs):
        parse_result = super().parse_args(*args, **kwargs)
        self._other.parsed = parse_result
        return parse_result


class FileContent(object):
    def __init__(self, file):
        self.file = file
        self._content = None

    def get_content(self):
        if self._content is None:
            content = just_read(self.file)
            self._content = json.loads(content)
            del content
        return self._content

    def delete(self):
        del self._content
        self._content = None


class BenchmarkPlot:
    name: str
    plot_args: Any
    parser: MaskedParser
    parsed: argparse.Namespace

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

    def __init__(self, name: str, ignore_args: bool = False):
        self.name = name
        self.plot_args = None
        # Makes a simple parser.
        # The caller can add more arguments if needed.
        parser = MaskedParser(self, prog=f"result_analyzer_{self.name}")
        if not ignore_args:
            parser.add_argument("-r", "--root", required=True, type=str)
            parser.add_argument("-f", "--files", required=False, type=str, nargs="+")
            parser.add_argument("--i", required=False, type=str)
        parser.add_argument("--out_dir", required=True, type=str)
        self.parser = parser
        current_timestamp = datetime.now()
        datetime_string = current_timestamp.strftime("%Y_%m_%d_%H_%M_%S")
        self.time = datetime_string

    def plot(self, **kwargs):
        self.plot_args = kwargs

    # a generator because why not.
    def read_files(self, file_name: str, callback: bool = False):
        parsed = self.parsed
        files_to_read = parsed.files
        if not files_to_read:
            assert parsed.i is not None
            norm_files = just_read(parsed.i)
            assert norm_files is not None
            files_to_read = [
                sl
                for sl in [l.strip() for l in norm_files.split("\n")]
                if len(sl) > 0 and not sl.startswith("#")
            ]

        for file in files_to_read:
            complete_path = f"{parsed.root}/{file}/{file_name}"
            paths = glob.glob(complete_path)
            for path in paths:
                assert Path(path).exists(), f"Expected {path} to exist!"
                # soooo turns out the JSON files can be 1 GIGA BYTE in size for TraceProv.
                # ofc, Python chokes on that (OOM errors when there are multiple files)
                # no, all the data needs to be strictly callback functions that the callers can decide whether worth
                # recording at once or not
                contents = FileContent(path)
                if not callback:
                    contents = contents.get_content()
                yield (path, contents)

    def add_timestamp(self):
        parsed = self.parsed
        db_name = parsed.db
        db_path = Path(db_name).name
        out_dir = Path(parsed.out_dir) / f"{db_path}_{self.name}"
        os.makedirs(out_dir, exist_ok=True)
        true_out_dir = out_dir / f"time_{self.time}"
        os.makedirs(true_out_dir, exist_ok=False)
        just_write(true_out_dir / "command.txt", " ".join(sys.argv))
        assert not hasattr(parsed, "_true_out_dir")
        setattr(parsed, "_true_out_dir", true_out_dir)
        return true_out_dir


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


def get_unique_handles_labels(plot_axis):
    handles_all = []

    handles, legend_labels = plot_axis.get_legend_handles_labels()
    print(handles, legend_labels)

    legend_labels_set = set(legend_labels)
    legend_label_ordered = sorted(
        list(legend_labels_set), key=lambda x: legend_labels.index(x)
    )
    handles_all.extend(
        handles[legend_labels.index(label)] for label in legend_label_ordered
    )
    return (legend_label_ordered, handles_all)


def setup_tpch_analyzer_parser(mpl):
    parser = argparse.ArgumentParser("tpch_analyzer")
    parser.add_argument("--dir", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--out_dir", required=True)
    parser.add_argument("--sf", required=True)
    parser.add_argument(
        "--poster_mode", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--use_cache", action=argparse.BooleanOptionalAction, default=False
    )

    parsed = parser.parse_args()
    if parsed.poster_mode:
        mpl.rcParams.update(
            {
                # fonts
                "font.family": "serif",
                "font.size": 16,
                "axes.labelsize": 18,
                "xtick.labelsize": 16,
                "ytick.labelsize": 16,
                "legend.fontsize": 15,
                "axes.titlesize": 18,
                # cleaner look
                "axes.spines.top": False,
                "axes.spines.right": False,
                # lines
                "lines.linewidth": 2,
                "patch.linewidth": 1.5,
            }
        )
    out_dir = Path(parsed.out_dir) / parsed.sf
    os.makedirs(out_dir, exist_ok=True)
    return parsed, out_dir


def slice_filter(raw_values, invalid_values):
    return [
        (0 if _idx in invalid_values else raw_value)
        for (_idx, raw_value) in enumerate(raw_values)
    ]


class QueryCategory(NamedTuple):
    label: str
    queries: list[int]


def query_categories():
    simple_scans_aggregations_small_joins_leq_3 = sorted([1, 3, 6, 12, 14, 19])
    scans_aggregations_larger_joins = sorted([10, 5, 9, 7, 8])
    uncorrelated_subqueries = sorted([15, 16, 18, 11])
    correlated_subqueries_complex_subqueries = sorted([4, 2, 13, 17, 20, 21, 22])
    CAT_LABEL_1 = "Simple Scans / \n Aggregations with # Joins < 3"
    CAT_LABEL_2 = "Simple Scans / \n Aggregations with wider joins"
    CAT_LABEL_3 = "Uncorrelated \n Subqueries"
    CAT_LABEL_4 = "Correlated Subqueries / \n Complex subqueries"

    cats = [
        QueryCategory(CAT_LABEL_1, simple_scans_aggregations_small_joins_leq_3),
        QueryCategory(CAT_LABEL_2, scans_aggregations_larger_joins),
        QueryCategory(CAT_LABEL_3, uncorrelated_subqueries),
        QueryCategory(CAT_LABEL_4, correlated_subqueries_complex_subqueries),
    ]
    return cats


def add_arrow_label(top_plot_axis, label, xpos=0, ypos=1.12, transform=None):
    if transform is None:
        transform = blended_transform_factory(
            top_plot_axis.transAxes, top_plot_axis.transAxes
        )

    top_plot_axis.annotate(
        "",
        xy=(0, 1.1),
        xycoords=transform,  # arrow head position (top)
        xytext=(0, 0.0),
        textcoords=transform,  # arrow tail position (bottom)
        arrowprops=dict(arrowstyle="->", color="black", lw=1.5),
    )
    top_plot_axis.set_ylabel(None)
    # Label at the top of the arrow
    top_plot_axis.text(
        xpos,
        ypos,
        label,
        transform=transform,
        ha="center",
        va="bottom",
        fontsize=10,
    )


def tp_add_grid_line(axis):
    axis.grid(
        visible=True,
        axis="y",
        which="major",
        color="gray",
        linestyle="--",
        linewidth=0.5,
        alpha=0.7,
    )


EXTRA_PREDICATE = "_EXTRA_PREDICATE_"


def replace_extra_predicate(query: str, extra_predicate: str):
    assert EXTRA_PREDICATE in query
    return query.replace(EXTRA_PREDICATE, extra_predicate)


def add_parallel_predicate(extra_predicates: str, parallel: int = None):
    if parallel is None:
        return extra_predicates
    return f'({extra_predicates}) and "parallel"={parallel}'


def make_list_query(in_query: str, extra_predicates: str, parallel: int = None):
    new_extra_predicate = add_parallel_predicate(extra_predicates, parallel)
    in_query = replace_extra_predicate(in_query, new_extra_predicate)
    group_by_clause = f"category"
    if parallel is None:
        group_by_clause = f"{group_by_clause}, parallel"
    return f"""
    select
        {group_by_clause},
        list(t)
    from
        ( select * from ({in_query})) t group by {group_by_clause}
    """


def time_func_formatter():
    def _formatter(val, _):
        val = float(val)
        if val.is_integer():
            return str(int(val))
        assert val < 1.0
        logged = -1 * math.floor(math.log(val, 10))
        return str(round(val, logged))

    return ticker.FuncFormatter(_formatter)
