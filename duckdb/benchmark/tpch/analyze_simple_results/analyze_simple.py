# the simpler handling case.

import argparse
from functools import reduce
import json
import math
import os
from pathlib import Path
import random
from typing import Tuple

from matplotlib import pyplot as plt
import matplotlib as mpl
import numpy as np

from traceprovpy.tools.duckdb_inference import DuckDBDriverOptions
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse
from traceprovpy.tools.file_utils import (
    get_breakpoint_labels,
    get_slowdown_cats,
    json_read_file,
    just_read,
    just_write,
    null_safe,
)
from traceprovpy.tools.normalized_row import (
    Extendable,
    Normalizable,
    NormalizedRow,
    extract_infer,
    make_dummy_profile_result,
    make_dummy_simple_result,
    tap_profile_result,
    tap_simple_result,
)
from traceprovpy.tools.plot_utils import (
    BenchmarkPlot,
    get_unique_handles_labels,
    query_categories,
    setup_tpch_analyzer_parser,
)
from traceprovpy.tools.run_duckdb_generic import add_query_options
import duckdb
import re
from matplotlib.transforms import blended_transform_factory

from matplotlib.gridspec import GridSpec


class NormalizedDuckTPCHRow(Normalizable):
    category: str
    query_num: str
    threads: int
    phase_1: Extendable
    phase_1_profile: Extendable
    phase_2: Extendable
    misc: Extendable

    def keys(self):
        return {
            "category",
            "query_num",
            "phase_1",
            "phase_1_profile",
            "phase_2",
            "threads",
            "misc"
        }


ERROR_CODES = {"errorcode", "timeout"}


def get_suffix(call_options):
    bench_parser = make_duckdb_parse()
    # add_query_options(bench_parser)
    bench_parsed, _ = bench_parser.parse_known_args(call_options)
    suffix = DuckDBDriverOptions.get_suffix(bench_parsed)
    return suffix, bench_parsed


def parse_category(category: str):
    if category == "SmokedDuck":
        return category
    if category == "base":
        return "base"
    cleaned = category.replace("optimized-y", "").replace("optimized-n", "").strip("__")
    option_split = cleaned.split("__")
    options = []
    for option in option_split:
        splitted = option.split("-")
        if len(splitted) != 2:
            continue
        option_name, option_value = splitted
        options.append(f"{option_name[0].capitalize()}-{option_value}")
    joined = ",".join(options)
    if "gprom" in category:
        prefix = option_split[0]
    else:
        prefix = "TraceProv"
    return f"{prefix} ({joined})"


class ResultAnalyzer:

    def __init__(self):
        self.added_base = set()

    def gprom(self, key: str, path: Path) -> list[NormalizedDuckTPCHRow]:
        print("Handling GProM!", key)
        result = json_read_file(path, True)
        query_results = result["results"]
        rows: list[NormalizedDuckTPCHRow] = []
        call_options = result["call_options"]
        suffix, bench_parsed = get_suffix(call_options)
        repeat = None
        for query_num, query_data in query_results.items():
            for category, _category_data in query_data.items():
                # print(_category_data)
                if isinstance(_category_data, list):
                    category_type = _category_data[0]['type']
                    category_data = _category_data[0]['infer']
                else:
                    category_data = _category_data
                    category_type = 'fail'
                if repeat is None:
                   repeat = len(category_data["time"])
                category_name = f"{category}_{suffix}"
                if "timeout" in category_data:
                    print("Handing timeout!")
                    rows.append(
                        NormalizedDuckTPCHRow(
                            category=category_name,
                            query_num=query_num,
                            phase_1=Extendable([make_dummy_simple_result(-1)] * repeat),
                            phase_1_profile=Extendable(
                                [make_dummy_profile_result(-1)] * repeat
                            ),
                            phase_2=None,
                            threads=bench_parsed.threads,
                            misc=None
                        )
                    )
                    continue
                elif "errorcode" in category_data:
                    rows.append(
                        NormalizedDuckTPCHRow(
                            category=category_name,
                            query_num=query_num,
                            phase_1=Extendable([make_dummy_simple_result(-2)] * repeat),
                            phase_1_profile=Extendable(
                                [make_dummy_profile_result(-2)] * repeat
                            ),
                            phase_2=None,
                            threads=bench_parsed.threads,
                            misc=None
                        )
                    )
                    continue
                base = list(map(tap_simple_result, category_data["time"]))
                base_profile = list(map(tap_profile_result, category_data["profile"]))
                rows.append(
                    NormalizedDuckTPCHRow(
                        category=category_name,
                        query_num=query_num,
                        phase_1=Extendable(base),
                        phase_1_profile=Extendable(base_profile),
                        phase_2=None,
                        threads=bench_parsed.threads,
                        misc=None
                    )
                )
        print("Len rows: ", len(rows))
        return rows

    def traceprov(
        self,
        key: str,
        path: Path,
    ) -> list[NormalizedDuckTPCHRow]:
        print("Handling TraceProv!", key)
        result = json_read_file(path, True)
        query_results = result["results"]
        rows: list[NormalizedDuckTPCHRow] = []
        base_rows: list[NormalizedDuckTPCHRow] = []
        bench_parser = make_duckdb_parse()
        add_query_options(bench_parser)
        call_options = result["call_options"]
        bench_parsed, _ = bench_parser.parse_known_args(call_options)
        suffix = DuckDBDriverOptions.get_suffix(bench_parsed)
        category_name = f"{key}_{suffix}"
        for query_num, query_data in query_results.items():
            query_result = query_data["result"]
            base = list(map(tap_simple_result, query_result["base_time"]))
            base_profile = list(map(tap_profile_result, query_result["base_profile"]))
            base_rows.append(
                NormalizedDuckTPCHRow(
                    category=f"base",
                    query_num=query_num,
                    phase_1=Extendable(base),
                    phase_1_profile=Extendable(base_profile),
                    phase_2=None,
                    threads=bench_parsed.threads,
                    misc=None
                )
            )
            phase_1 = list(map(tap_simple_result, query_result["capture_time"]))
            phase_1_profile = list(
                map(tap_profile_result, query_result["capture_profile"])
            )
            phase_2 = extract_infer(query_result["infer_results"])
            rows.append(
                NormalizedDuckTPCHRow(
                    category=category_name,
                    query_num=query_num,
                    phase_1=Extendable(phase_1),
                    phase_1_profile=Extendable(phase_1_profile),
                    phase_2=Extendable(phase_2),
                    threads=bench_parsed.threads,
                    misc=None
                )
            )
        all_rows = [*rows]
        base_key = ("base", bench_parsed.threads)
        if base_key not in self.added_base:
            all_rows.extend(base_rows)
        self.added_base.add(base_key)
        return all_rows

    def sd(self, key: str, path: Path) -> list[NormalizedDuckTPCHRow]:
        print("Handling SmokedDuck", key)
        result = json_read_file(path, True)
        query_results = result["results"]
        rows: list[NormalizedDuckTPCHRow] = []
        # base_rows: list[NormalizedDuckTPCHRow] = []
        call_options = result["call_options"]
        suffix, bench_parsed = get_suffix(call_options)
        for query_num, query_data in query_results.items():
            query_core_result = query_data["result"]["sd"]
            # base = list(map(tap_simple_result, query_core_result["base_time"]))
            # base_profile = list(
            #     map(tap_profile_result, query_core_result["base_profile"])
            # )
            # base_rows.append(
            #     NormalizedDuckTPCHRow(
            #         category=f"base",
            #         query_num=query_num,
            #         phase_1=Extendable(base),
            #         phase_1_profile=Extendable(base_profile),
            #         phase_2=None,
            #         threads=1,
            #         misc=None,
            #     )
            # )
            if query_core_result["capture_time"] is None:
                continue
            phase_1 = list(map(tap_simple_result, query_core_result["capture_time"]))
            phase_1_profile = list(
                map(tap_profile_result, query_core_result["capture_profile"])
            )
            phase_2 = extract_infer(query_core_result["infer_results"])
            capture_stats = query_core_result["capture_stats"]
            reduced_stats = [
                dict(
                    log_size_bytes=stats["size_mb"],
                    postprocess_time=stats["postprocess_time"],
                    build_time=stats["build_time"],
                )
                for stats in capture_stats
            ]
            rows.append(
                NormalizedDuckTPCHRow(
                    category="SmokedDuck",
                    query_num=query_num,
                    phase_1=Extendable(phase_1),
                    phase_1_profile=Extendable(phase_1_profile),
                    phase_2=Extendable(phase_2),
                    threads=bench_parsed.threads,
                    misc=Extendable(reduced_stats),
                )
            )
        all_rows = [*rows]
        return all_rows


def plot_data(
    data: list[Tuple[str, dict]],
    categories: list[str],
    label: str,
    sf: str,
    out_dir: Path,
    width: float,
    group_gap: float,
    category_label_mapping: dict = None,
    fig_size=(14, 3),
):
    x_axis_values = list(map(str, range(1, 23)))
    x_axis = np.arange(len(x_axis_values))
    slowdown_fig, slowdown_axis = plt.subplots(1, 1, figsize=fig_size)
    data = [item for item in data if item[0] in categories]
    result_sorted = sorted(data, key=lambda x: categories.index(x[0]))
    pending_bars = []
    for category_idx, (category, catagory_data) in enumerate(result_sorted):
        x_axis_adjusted = (
            x_axis * (len(categories) * width + group_gap) + width * category_idx
        )

        def _get_key_in_dict(in_key: str):
            return null_safe(
                [
                    catagory_data[q][in_key] if q in catagory_data else 0
                    for q in x_axis_values
                ]
            )

        phase_all_slowdown = _get_key_in_dict("phase_all_slowdown")
        y_values = _get_key_in_dict("phase_1_explain_time_median")
        y_values_error = _get_key_in_dict("phase_1_explain_time_stdev")

        def _filter_error(in_list):
            return [
                0 if (idx in [*y_error_values, *y_timeout_values]) else val
                for (idx, val) in enumerate(in_list)
            ]

        y_error_values = [
            y_idx for y_idx, y in enumerate(y_values) if -2.5 < y and y < -1.5
        ]
        y_timeout_values = [
            y_idx for y_idx, y in enumerate(y_values) if -1.5 < y and y < -0.5
        ]
        print(y_error_values, y_timeout_values)
        print(y_values)
        phase_all_slowdown = [
            0 if (idx in [*y_error_values, *y_timeout_values]) else val
            for (idx, val) in enumerate(phase_all_slowdown)
        ]
        phase_1_time = _filter_error(y_values)
        phase_1_time_error = _filter_error(y_values_error)

        slowdown_axis.bar(
            x_axis_adjusted,
            phase_all_slowdown,
            width=width,
            label=(category_label_mapping or dict()).get(category, category),
            color=BenchmarkPlot.colors[category_idx],
        )

        top_plot_axis.bar(
            x_axis_values,
            phase_1_time,
            width=width,
            label=(category_label_mapping or dict()).get(category, category),
            color=BenchmarkPlot.colors[category_idx],
            yerr=phase_1_time_error,
        )
        # if y_error_values:
        #     slowdown_axis.bar(
        #         [x_axis_adjusted[idx] for idx in y_error_values],
        #         10,
        #         width=width,
        #         label="Error",
        #         color="lightgray",
        #         hatch="///",
        #         edgecolor="blue",
        #     )
        if y_timeout_values:
            pending_bars.append(
                dict(
                    x=[x_axis_adjusted[idx] for idx in y_timeout_values],
                    height=10**5,
                    width=width,
                    label="Timeout",
                    color="lightgray",
                    hatch="///",
                    edgecolor="red",
                )
            )

    # for bar in pending_bars:
    #     slowdown_axis.bar(**bar)
    slowdown_axis.legend(loc="upper left", bbox_to_anchor=(0, 1.2))

    slowdown_axis.set_ylim(bottom=0.5, top=300)
    slowdown_axis.annotate(
        "~525x",
        xy=(
            x_axis_values.index("11") * (len(categories) * width + group_gap) + 3.5,
            50,
        ),
        ha="center",
        fontsize=14,
        color="red",
        fontweight="bold",
    )
    slowdown_axis.annotate(
        "~1060x",
        xy=(
            x_axis_values.index("22") * (len(categories) * width + group_gap) + 3,
            50,
        ),
        ha="center",
        fontsize=14,
        color="red",
        fontweight="bold",
    )

    slowdown_axis.axhline(y=1, color="r", linestyle="--")
    slowdown_axis.axhline(y=2, color="r", linestyle="--")
    slowdown_axis.set_ylabel("Slowdown")
    slowdown_axis.set_xlabel("Query")
    slowdown_axis.set_yscale("log")
    slowdown_axis.set_xticks(
        x_axis * (len(categories) * width + group_gap) + width * (len(categories) / 3),
        x_axis_values,
    )
    slowdown_fig.suptitle(f"DuckDB Slowdown for SF={sf}")
    slowdown_fig.savefig(out_dir / f"{label}_slowdown.pdf", bbox_inches="tight")
    time_plot_fig.savefig(out_dir / f"{label}_phase_1_time.pdf", bbox_inches="tight")


ALL_CATEGORIES = [
    "base",
    "gprom_join_heuristics_optimized-y__threads-1",
    "gprom_join_heuristics_optimized-y__threads-2",
    "gprom_join_heuristics_optimized-y__threads-4",
    "gprom_join_heuristics_optimized-y__threads-8",
    "gprom_join_heuristics_optimized-y__threads-10",
    "gprom_join_heuristics_optimized-y__threads-12",
    "gprom_join_optimized-y__threads-1",
    "gprom_join_optimized-y__threads-2",
    "gprom_join_optimized-y__threads-4",
    "gprom_join_optimized-y__threads-8",
    "gprom_join_optimized-y__threads-10",
    "gprom_join_optimized-y__threads-12",
    "gprom_window_heuristics_optimized-y__threads-1",
    "gprom_window_heuristics_optimized-y__threads-2",
    "gprom_window_heuristics_optimized-y__threads-4",
    "gprom_window_heuristics_optimized-y__threads-8",
    "gprom_window_heuristics_optimized-y__threads-10",
    "gprom_window_heuristics_optimized-y__threads-12",
    "gprom_window_optimized-y__threads-1",
    "gprom_window_optimized-y__threads-2",
    "gprom_window_optimized-y__threads-4",
    "gprom_window_optimized-y__threads-8",
    "gprom_window_optimized-y__threads-10",
    "gprom_window_optimized-y__threads-12",
    "traceprov_optimized-y__threads-1__compact-y__merge_chunks-y",
    "traceprov_optimized-y__threads-1__compact-y__merge_chunks-y__table_stats-y",
    "traceprov_optimized-y__threads-2__compact-y__merge_chunks-y",
    "traceprov_optimized-y__threads-2__compact-y__merge_chunks-y__table_stats-y",
    "traceprov_optimized-y__threads-4__compact-y__merge_chunks-y",
    "traceprov_optimized-y__threads-4__compact-y__merge_chunks-y__table_stats-y",
    "traceprov_optimized-y__threads-8__compact-y__merge_chunks-y",
    "traceprov_optimized-y__threads-8__compact-y__merge_chunks-y__table_stats-y",
    "traceprov_optimized-y__threads-10__compact-y__merge_chunks-y",
    "traceprov_optimized-y__threads-10__compact-y__merge_chunks-y__table_stats-y",
    "traceprov_optimized-y__threads-12__compact-y__merge_chunks-y",
    "traceprov_optimized-y__threads-12__compact-y__merge_chunks-y__table_stats-y",
]

BASE_CASES = [
    ("base", 1),
    ("base", 2),
    ("base", 4),
    ("base", 8),
    ("base", 10),
    ("base", 12),
]
GPROM_CASES = {
    "base": BASE_CASES,
    "gprom_join": [
        "gprom_join_optimized-y__threads-1",
        "gprom_join_optimized-y__threads-2",
        "gprom_join_optimized-y__threads-4",
        "gprom_join_optimized-y__threads-8",
        "gprom_join_optimized-y__threads-10",
        "gprom_join_optimized-y__threads-12",
    ],
    "gprom_join_heuristics": [
        "gprom_join_heuristics_optimized-y__threads-1",
        "gprom_join_heuristics_optimized-y__threads-2",
        "gprom_join_heuristics_optimized-y__threads-4",
        "gprom_join_heuristics_optimized-y__threads-8",
        "gprom_join_heuristics_optimized-y__threads-10",
        "gprom_join_heuristics_optimized-y__threads-12",
    ],
    "gprom_window": [
        "gprom_window_optimized-y__threads-1",
        "gprom_window_optimized-y__threads-2",
        "gprom_window_optimized-y__threads-4",
        "gprom_window_optimized-y__threads-8",
        "gprom_window_optimized-y__threads-10",
        "gprom_window_optimized-y__threads-12",
    ],
    "gprom_window_heuristics": [
        "gprom_window_heuristics_optimized-y__threads-1",
        "gprom_window_heuristics_optimized-y__threads-2",
        "gprom_window_heuristics_optimized-y__threads-4",
        "gprom_window_heuristics_optimized-y__threads-8",
        "gprom_window_heuristics_optimized-y__threads-10",
        "gprom_window_heuristics_optimized-y__threads-12",
    ],
}

TRACEPROV_OPTIMIZED_CASES = {
    "base": BASE_CASES,
    "traceprov": [
        "traceprov_optimized-y__threads-1__compact-y__merge_chunks-y",
        "traceprov_optimized-y__threads-2__compact-y__merge_chunks-y",
        "traceprov_optimized-y__threads-4__compact-y__merge_chunks-y",
        "traceprov_optimized-y__threads-8__compact-y__merge_chunks-y",
        "traceprov_optimized-y__threads-10__compact-y__merge_chunks-y",
        "traceprov_optimized-y__threads-12__compact-y__merge_chunks-y",
    ],
    "traceprov_stats": [
        "traceprov_optimized-y__threads-1__compact-y__merge_chunks-y__table_stats-y",
        "traceprov_optimized-y__threads-2__compact-y__merge_chunks-y__table_stats-y",
        "traceprov_optimized-y__threads-4__compact-y__merge_chunks-y__table_stats-y",
        "traceprov_optimized-y__threads-8__compact-y__merge_chunks-y__table_stats-y",
        "traceprov_optimized-y__threads-10__compact-y__merge_chunks-y__table_stats-y",
        "traceprov_optimized-y__threads-12__compact-y__merge_chunks-y__table_stats-y",
    ],
}


def gen_time_plots(
    original_data: list[Tuple[str, dict]],
    data_spec,
    label: str,
    sf: str,
    out_dir: Path,
    width: float,
    legend_labels: list[str],
    value_data_key,
):
    return
    x_axis_values = list(map(str, range(1, 23)))
    x_axis = np.arange(len(x_axis_values))
    time_plot_fig, top_plot_axis = plt.subplots(
        len(data_spec), 1, figsize=(14, 8), gridspec_kw={"hspace": 0.5, "wspace": 0}
    )
    # time_plot_fig = plt.figure()
    # gs = time_plot_fig.add_gridspec(len(data_spec), hspace=1)
    # top_plot_axis = gs.subplots(sharex=True, sharey=False)
    for axis_idx, (data_key, data_values) in enumerate(data_spec.items()):
        data = []
        for item in data_values:
            print("TRYING TO FIND", item)
            for x in original_data:
                print("VALUE", x[0])
            if isinstance(item, str):
                # print(original_data)
                filtered = [x for x in original_data if x[0][0] == item][0]
            else:
                # print(original_data)
                filtered = [x for x in original_data if x[0] == item][0]
            data.append(filtered)
        result_sorted = data
        print(len(result_sorted), len(data_values))
        for category_idx, (combined_category, category_data) in enumerate(
            result_sorted
        ):
            category = combined_category[0]
            category_threads = combined_category[1]

            def _get_key_in_dict(in_key: str):
                return null_safe(
                    [
                        category_data[q][in_key] if q in category_data else 0
                        for q in x_axis_values
                    ]
                )

            y_values = _get_key_in_dict(f"{value_data_key}_median")
            y_values_error = _get_key_in_dict(f"{value_data_key}_stdev")

            y_error_values = [
                y_idx for y_idx, y in enumerate(y_values) if -2.5 < y and y < -1.5
            ]
            y_timeout_values = [
                y_idx for y_idx, y in enumerate(y_values) if -1.5 < y and y < -0.5
            ]
            print("LEN ERROR VALUES: ", y_error_values)
            print("LEN TIMEOUT VALUES: ", y_timeout_values)

            def _filter_error(in_list):
                return [
                    0 if (idx in [*y_error_values, *y_timeout_values]) else val
                    for (idx, val) in enumerate(in_list)
                ]

            phase_1_time = _filter_error(y_values)
            phase_1_time_error = _filter_error(y_values_error)
            raw_category = (None or dict()).get(category, category)
            parsed = parse_category(raw_category)
            x_axis_shifted = x_axis + width * category_idx
            top_plot_axis[axis_idx].bar(
                x_axis_shifted,
                phase_1_time,
                width=width,
                label=legend_labels[category_idx],
                color=BenchmarkPlot.colors[category_idx],
                yerr=phase_1_time_error,
            )
            print(data_key, combined_category, phase_1_time)
            top_plot_axis[axis_idx].set_yscale("log", base=10)
            top_plot_axis[axis_idx].set_title(data_key)
            top_plot_axis[axis_idx].set_xticks(
                x_axis + width * (len(data_values) / 2),
                x_axis_values,
            )
    top_plot_axis[axis_idx].legend(loc="center right", bbox_to_anchor=(1.10, 1))
    time_plot_fig.suptitle(label)
    time_plot_fig.savefig(out_dir / f"{label}_time.pdf")


DATA_SPEC_THREADS = lambda threads_count: [
    ("base", threads_count),
    # f"gprom_join_optimized-n__threads-{threads_count}",
    # f"gprom_join_heuristics_optimized-n__threads-{threads_count}",
    # f"gprom_window_optimized-n__threads-{threads_count}",
    f"traceprov_optimized-y__threads-{threads_count}__compact-y__merge_chunks-y__table_stats-y",
    f"gprom_window_heuristics_optimized-y__threads-{threads_count}",
    "SmokedDuck",
]


def replace_thread(in_str: str):
    reg = r"__threads-\d+"
    result = re.sub(reg, "", in_str)
    print("Mapping", in_str, result)
    return result


def filter_data(original_data, item):
    print("searching for", item)
    if isinstance(item, str):
        filtered = [x for x in original_data if x[0][0] == item][0]
    else:
        filtered = [x for x in original_data if x[0] == item][0]
    return filtered


def gen_generate_shared_plots(
    thread_count,
    original_data: list[Tuple[str, dict]],
    width: float,
    out_dir: Path,
    labels,
    categories,
    title,
    ylabel,
    value_data_key,
    value_median_key,
    needs_extra=False,
):
    # width = 0.
    x_axis_categories = query_categories()
    x_axis_combined = [cat.queries for cat in x_axis_categories]
    x_axis_values = [str(_query) for _queries in x_axis_combined for _query in _queries]
    assert len(x_axis_values) == 22 and len(set(x_axis_values)) == 22
    # x_axis_values = list(map(str, range(1, 23)))
    # gap_multiplier = 1.1
    x_axis = np.arange(len(x_axis_values))

    time_plot_fig, top_plot_axis = plt.subplots(
        1, 1, figsize=(13, 2), gridspec_kw={"hspace": 0.5, "wspace": 0}
    )
    data = []
    for item in categories:
        filtered = filter_data(original_data, item)
        data.append(filtered)
    timeout_max = 0
    pending_bars = []
    timeout_label = "Timeout"
    errorout_label = "Error"
    MARGIN = 0.0
    top_plot_axis.margins(x=MARGIN)
    for category_idx, (combined_category, category_data) in enumerate(data):
        category = combined_category[0]

        def _get_key_in_dict(in_key: str):
            return null_safe(
                [
                    category_data[q][in_key] if q in category_data else 0
                    for q in x_axis_values
                ]
            )

        y_values = _get_key_in_dict(value_data_key)
        if value_median_key:
            y_values_error = _get_key_in_dict(value_median_key)
        else:
            y_values_error = None

        y_values_time_check = _get_key_in_dict("phase_1_explain_time_median")
        y_error_values = [
            y_idx
            for y_idx, y in enumerate(y_values_time_check)
            if -2.5 < y and y < -1.5
        ]
        y_timeout_values = [
            y_idx
            for y_idx, y in enumerate(y_values_time_check)
            if -1.5 < y and y < -0.5
        ]
        timeout_max = max(timeout_max, *y_values)

        def _filter_error(in_list):
            return [
                0 if (idx in [*y_error_values, *y_timeout_values]) else val
                for (idx, val) in enumerate(in_list)
            ]

        phase_1_time = _filter_error(y_values)
        phase_1_time_error = (
            _filter_error(y_values_error) if y_values_error else y_values_error
        )
        raw_category = (None or dict()).get(category, category)
        x_axis_shifted = x_axis + width * category_idx
        top_plot_axis.bar(
            x_axis_shifted,
            phase_1_time,
            width=width,
            label=labels[category_idx],
            color=BenchmarkPlot.colors[category_idx],
            yerr=phase_1_time_error,
        )
        if y_timeout_values:
            pending_bars.append(
                dict(
                    x=[x_axis_shifted[idx] for idx in y_timeout_values],
                    height=0,
                    width=width,
                    label=timeout_label,
                    color="lightgray",
                    hatch="///",
                    edgecolor="red",
                )
            )
        if y_error_values:
            pending_bars.append(
                dict(
                    x=[x_axis_shifted[idx] for idx in y_error_values],
                    height=0,
                    width=width,
                    label=errorout_label,
                    color="lightgray",
                    hatch="...",
                    edgecolor="red",
                )
            )
    if needs_extra:
        top_plot_axis.axhline(y=1, color="r", linestyle="--")
        top_plot_axis.axhline(y=2, color="r", linestyle="--")
    top_plot_axis.set_yscale("log", base=10)
    # time_plot_fig.suptitle(title)
    top_plot_axis.set_xticks(
        x_axis + width*1.5,
        x_axis_values,
    )
    top_plot_axis.set_xlabel("Query")
    # top_plot_axis.set_ylabel(ylabel)
    for bar in pending_bars:
        bar["height"] = timeout_max
        top_plot_axis.bar(**bar)

    legend_labels_all, handles_all = get_unique_handles_labels(top_plot_axis)
    running = 0
    trans = blended_transform_factory(top_plot_axis.transData, top_plot_axis.transAxes)

    colors = ["#d6d6d6", 'white']
    for _idx, query_category in enumerate(x_axis_combined):
        old_running = running
        running += len(query_category)
        if _idx < len(x_axis_combined) - 1:
            top_plot_axis.vlines(x=running-width - 0.00, colors='black', linestyles='solid', ymin=0, ymax=1_000_000 * timeout_max, linewidth=1)
        new_running = running
        middle = int((old_running + new_running) / 2)
        top_plot_axis.text(middle, 1.02, x_axis_categories[_idx].label, transform=trans, ha='center', va='bottom', fontsize=9)
        start_ = old_running-width
        if _idx == 0:
            start_ -= 0.1
        top_plot_axis.axvspan(start_, new_running-width, color=colors[_idx % 2], zorder=0)

    top_plot_axis.legend(
        handles=handles_all,
        labels=legend_labels_all,
        loc="center right",
        bbox_to_anchor=(1.10, 0.95),
        prop=dict(size=9),
    )

    top_plot_axis.grid(visible=True, axis='y', which='major',color='gray', linestyle='--', linewidth=0.5, alpha=0.7)
    y_values_ticks = [10**(-2), 10**(-1), 1]
    y_value_labels = ["0.01", "0.1", "1"]
    top_plot_axis.set_yticks(y_values_ticks)
    top_plot_axis.set_yticklabels(y_value_labels)

    trans2 = blended_transform_factory(top_plot_axis.transAxes, top_plot_axis.transAxes)
    top_plot_axis.annotate(
        '', 
        xy=(0, 1.1), xycoords=trans2,      # arrow head position (top)
        xytext=(0, 0.0), textcoords=trans2, # arrow tail position (bottom)
        arrowprops=dict(arrowstyle='->', color='black', lw=1.5)
    )

    # Label at the top of the arrow
    top_plot_axis.text(0, 1.12, "Total Time (s)", transform=top_plot_axis.transAxes, ha='center', va='bottom', fontsize=10)

    time_plot_fig.savefig(
        out_dir / f"end_to_end_comparison_{thread_count}_{value_data_key}.pdf",
        bbox_inches="tight",
    )


def plot_scalability_plot(
    raw_scalability: dict[int, dict[str, dict]],
    out_dir: Path,
    categories: list[str],
    labels: str,
):

    scalability = {
        key: {
            category: category_data
            for category, category_data in value.items()
            if category in categories
        }
        for (key, value) in raw_scalability.items()
    }
    # Figure out the max slowdown per query.
    traceprov_cat = categories[1]
    gprom_cat = categories[2]
    print("Using traceprov Cat: ", traceprov_cat)
    print(scalability)
    flattened = [(query, print('query data', query_data) or query_data['phase_all_slowdown']) for cat_result in scalability.values() for query, query_data in cat_result[traceprov_cat].items()]

    def _get_max(prev, curr):
        _query, _query_slowdown = curr
        max_value = _query_slowdown
        if _query in prev:
            max_value = max(max_value, prev[_query])
        return {**prev, _query: max_value }
    max_slowdown_per_query = reduce(_get_max, flattened, dict())
    break_points = [1.2, 1.4]
    slowdown_cats = get_slowdown_cats(max_slowdown_per_query.items(), break_points)
    breakpoint_labels = get_breakpoint_labels(break_points, add_percent=False)
    x_axis_values = [cell for node in slowdown_cats for cell in node]
    print(max_slowdown_per_query)
    
    combined_figure = plt.figure(figsize=(14, 3.5))
    gs = GridSpec(2, 2, figure=combined_figure, hspace=0.4)
    # scalability_figure, scalability_axis = plt.subplots(1, 1, )
    scalability_axis = combined_figure.add_subplot(gs[0, :])
    # scalability_axis.set_title("End-to-end Runtime (s)")
    # x_axis_values = list(map(str, range(1, 23)))
    group_gap = 5.2
    x_axis = np.arange(len(x_axis_values)) * (1 + group_gap)
    width = 1.3
    markers = ["o", "^", "s"]
    y_line_axis = []
    threads = []
    for thread_idx, (thread, thread_data) in enumerate(
        sorted(scalability.items(), key=lambda x: x[0])
    ):
        threads.append(thread)
        for cat_idx, (category, category_data) in enumerate(
            sorted(thread_data.items(), key=lambda x: categories.index(x[0]))
        ):
            return_value = scalability_axis.scatter(
                x_axis + width * thread_idx,
                [(category_data.get(x, dict()).get('phase_total_time_median')) for x in x_axis_values],
                label=labels[categories.index(category)],
                marker=markers[categories.index(category)],
                # c=BenchmarkPlot.colors[thread_idx],
                c=BenchmarkPlot.colors[cat_idx],
                s=50,
                # s=80 if cat_idx < 2 else 70,
            )
            if cat_idx == 0:
                for offsets in return_value.get_offsets():
                    vline = scalability_axis.axvline(
                        offsets[0], linewidth=0.05, color="black"
                    )
                print(vline.get_xydata())
                y_line_axis.append(
                    (return_value.get_offsets()[0][0], vline.get_ydata(False)[0])
                )
    # scalability_axis.axhline(y=1, color="r", linestyle="--", linewidth=0.3)
    # scalability_axis.axhline(y=2, color="r", linestyle="--", linewidth=0.3)
    # scalability_axis.axhline(y=10, color="r", linestyle="--", linewidth=0.3)
    # scalability_axis.axhline(y=100, color="r", linestyle="--", linewidth=0.3)
    scalability_axis.set_xticks(x_axis + width * (len(scalability) / 2), x_axis_values)
    scalability_axis.set_yscale("log")
    legend_labels_all, handles_all = get_unique_handles_labels(scalability_axis)
    for _idx, points in enumerate(y_line_axis):
        print(points)
        scalability_axis.annotate(
            f"{threads[_idx]}", (points[0] - 0.75, 1100), rotation=0, fontsize=8
        )
    scalability_axis.legend(
        handles=handles_all,
        labels=legend_labels_all,
        # bbox_to_anchor=(1.10, 0.95),
        prop=dict(size=8),
    )
    colors = ["#d6d6d6", 'white']
    # scalability_axis.margins(0.0)
    x_axis_augmented = np.arange(len(x_axis_values)+1) * (1 + group_gap)
    for _idx, x_span in enumerate(x_axis_augmented[:-1]):
        _start = x_span-width
        if _idx == 0:
            _start -= 0.1
        scalability_axis.axvspan(_start, x_axis_augmented[_idx+1], color=colors[_idx % 2], zorder=0)
    scalability_axis.margins(x=0.0)
    trans2 = blended_transform_factory(scalability_axis.transAxes, scalability_axis.transAxes)
    scalability_axis.annotate(
        '', 
        xy=(0, 1.1), xycoords=trans2,      # arrow head position (top)
        xytext=(0, 0.0), textcoords=trans2, # arrow tail position (bottom)
        arrowprops=dict(arrowstyle='->', color='black', lw=1.5)
    )

    # Label at the top of the arrow
    scalability_axis.text(0, 1.12, "Total Time (s)", transform=scalability_axis.transAxes, ha='center', va='bottom', fontsize=10)
    running_len = 0
    trans = blended_transform_factory(scalability_axis.transData, scalability_axis.transAxes)
    for _idx, slowdown_cat in enumerate(slowdown_cats):
        old_running = running_len
        running_len += len(slowdown_cat)
        middle = int((old_running + running_len) / 2)
        middle = (group_gap  + 1)* middle
        scalability_axis.text(middle, 1.02, breakpoint_labels[_idx], transform=trans, ha='center', va='bottom', fontsize=9, ma='center')
        if _idx < len(slowdown_cats) - 1:
            scalability_axis.axvline(x=running_len*(1+group_gap) - 1, linestyle='solid', color='black', linewidth=4)
    scalability_axis.grid(visible=True, axis='y', which='major',color='gray', linestyle='--', linewidth=0.5, alpha=0.7)
    reduced_slowdown = combined_figure.add_subplot(gs[1, 0])
    red_slowdown_cats = [
        "TraceProv (I)",
        "TraceProv (I + II)",
        # "GProM"
    ]
    red_slowdown_queries = slowdown_cats[-1]
    print("slowdown queries: ", red_slowdown_queries)
    red_slowdown_data = dict()
    def _get_data(in_category, out_category, data_key, query_list=None):
        query_list = red_slowdown_queries if query_list is None else query_list
        return {
            key: {
                out_category: [ 0 if val < 0 else val for val in [value[in_category][q][data_key] for q in query_list]]
            }
            for (key, value) in scalability.items()
        }
    def merge_data(prev, curr):
        assert len(prev) != 0 or len(curr) != 0
        if len(prev) != 0:
            prev, curr = curr, prev
        if len(prev) == 0: return curr
        assert prev.keys() == curr.keys()
        return {
            key: {**prev[key], **curr[key]}
            for key in prev.keys()
        }
    red_slowdown_data = merge_data(_get_data(traceprov_cat, red_slowdown_cats[0], "phase_1_slowdown"), dict())
    red_slowdown_data = merge_data(_get_data(traceprov_cat, red_slowdown_cats[1], "phase_all_slowdown"), red_slowdown_data)
    # red_slowdown_data = merge_data(_get_data(gprom_cat, red_slowdown_cats[2], "phase_all_slowdown"), red_slowdown_data)
    red_x_axis = np.arange(len(red_slowdown_queries)) * (1 + group_gap)
    for thread_idx, (thread, thread_data) in enumerate(
        sorted(red_slowdown_data.items(), key=lambda x: x[0])
    ):
        threads.append(thread)
        for cat_idx, (category, category_data) in enumerate(
            sorted(thread_data.items(), key=lambda x: red_slowdown_cats.index(x[0]))
        ):
            print("Plotting cat: ", category, cat_idx, category_data)
            if all(x < 0 for x in category_data): continue
            return_value = reduced_slowdown.scatter(
                red_x_axis + width * thread_idx,
                category_data,
                label=category,
                marker=markers[cat_idx],
                # c=BenchmarkPlot.colors[thread_idx],
                c=BenchmarkPlot.colors[cat_idx],
                s=50,
                # s=80 if cat_idx < 2 else 70,
            )
            if cat_idx == 0:
                for offsets in return_value.get_offsets():
                    vline = reduced_slowdown.axvline(
                        offsets[0], linewidth=0.05, color="black"
                    )
                # print(vline.get_xydata())
                y_line_axis.append(
                    (return_value.get_offsets()[0][0], vline.get_ydata(False)[0])
                )
    legend_labels_all, handles_all = get_unique_handles_labels(reduced_slowdown)
    for _idx, points in enumerate(y_line_axis):
        print(points)
        reduced_slowdown.annotate(
            f"{threads[_idx]}", (points[0] - 0.75, 1100), rotation=0, fontsize=8
        )
    reduced_slowdown.legend(
        handles=handles_all,
        labels=legend_labels_all,
        # bbox_to_anchor=(1.10, 0.95),
        prop=dict(size=8),
    )
    print(red_slowdown_data)
    # reduced_slowdown.legend()
    reduced_slowdown.set_xticks(red_x_axis + width * (len(scalability) / 3), red_slowdown_queries)
    # reduced_slowdown.set_yscale("log")
    reduced_slowdown.axhline(y=1, color="r", linestyle="--", linewidth=0.3)
    reduced_slowdown.axhline(y=2, color="r", linestyle="--", linewidth=0.3)
    reduced_backtrace_time = combined_figure.add_subplot(gs[1, 1])
    reduced_slowdown.set_title("Slowdown")

    red_backtrace_cat = "TraceProv (II)"
    red_backtrace_cat_queries = [q for _cat in slowdown_cats for q in random.sample(_cat, min(len(_cat), 8))]
    red_backtrace_data = merge_data(_get_data(traceprov_cat, red_backtrace_cat, "phase_2_time_median", red_backtrace_cat_queries), dict())
    reduced_backtrace_time.set_title("Backtrace Time (s)")
    red_backtrace_x_axis = np.arange(len(red_backtrace_cat_queries)) * (1 + group_gap)
    for thread_idx, (thread, thread_data) in enumerate(
        sorted(red_backtrace_data.items(), key=lambda x: x[0])
    ):
        threads.append(thread)
        # print("Plotting cat: ", category, cat_idx, category_data)
        if all(x < 0 for x in category_data): continue
        return_value = reduced_backtrace_time.scatter(
            red_backtrace_x_axis + width * thread_idx,
            thread_data[red_backtrace_cat],
            label=category,
            marker=markers[cat_idx],
            # c=BenchmarkPlot.colors[thread_idx],
            c=BenchmarkPlot.colors[cat_idx],
            s=50,
            # s=80 if cat_idx < 2 else 70,
        )
        for offsets in return_value.get_offsets():
            vline = reduced_backtrace_time.axvline(
                offsets[0], linewidth=0.05, color="black"
            )
        # print(vline.get_xydata())
        y_line_axis.append(
            (return_value.get_offsets()[0][0], vline.get_ydata(False)[0])
        )
    reduced_backtrace_time.set_yscale("log")
    reduced_backtrace_time.set_xticks(red_backtrace_x_axis + width * (len(scalability) / 3), red_backtrace_cat_queries)
    # figure out which queries have the maximum slowdown of more than 1.4 >
    combined_figure.savefig(out_dir / "duckdb_scalability_time.pdf", bbox_inches="tight")


def gen_plots(database_file: Path, out_dir: Path, sf):
    con = duckdb.connect(database_file)
    cursor = con.cursor()
    query = """
    SELECT category, threads, list(normalized_slowdown) AS rows
    FROM normalized_slowdown
    GROUP BY category, threads;
    """
    cursor.execute(query)
    results = cursor.fetchall()
    cursor.close()
    con.close()
    result_mapped = list(
        [
            (
                (key, threads),
                {query_data["query_num"]: query_data for query_data in data},
            )
            for (key, threads, data) in results
        ]
    )
    print("ALL KEYS")
    print(list(key[0] for key in result_mapped))
    interesting_labels = [
        # "GProM Join",
        # "GProM Join Heu.",
        # "GProM Window",
        # "GProM Window Heu.",
        # "SmokedDuck",
        # "TraceProv Stats",
        "TraceProv",
        "GProM",
        "SmokedDuck"
    ]
    # gen_time_plots(
    #     result_mapped,
    #     GPROM_CASES,
    #     "gprom_all",
    #     sf,
    #     out_dir,
    #     0.1,
    #     [
    #         "(Thread 1)",
    #         "(Thread 2)",
    #         "(Thread 4)",
    #         "(Thread 8)",
    #         "(Thread 10)",
    #         "(Thread 12)",
    #     ],
    #     value_data_key="phase_1_explain_time",
    # )
    # gen_time_plots(
    #     result_mapped,
    #     TRACEPROV_OPTIMIZED_CASES,
    #     "traceprov_capture",
    #     sf,
    #     out_dir,
    #     0.1,
    #     [
    #         "(Thread 1)",
    #         "(Thread 2)",
    #         "(Thread 4)",
    #         "(Thread 8)",
    #         "(Thread 10)",
    #         "(Thread 12)",
    #     ],
    #     value_data_key="phase_1_explain_time",
    # )
    # gen_time_plots(
    #     result_mapped,
    #     TRACEPROV_OPTIMIZED_CASES,
    #     "traceprov_end_to_end",
    #     sf,
    #     out_dir,
    #     0.1,
    #     [
    #         "(Thread 1)",
    #         "(Thread 2)",
    #         "(Thread 4)",
    #         "(Thread 8)",
    #         "(Thread 10)",
    #         "(Thread 12)",
    #     ],
    #     value_data_key="phase_total_time",
    # )
    # interesting_labels = [
    #     # "GProM Join",
    #     # "GProM Join Heu.",
    #     # "GProM Window",
    #     # "GProM Window Heu.",
    #     # "SmokedDuck",
    #     # "TraceProv Stats",
    #     "TraceProv",
    #     "GProM",
    #     "SmokedDuck"
    # ]
    # threads = [1, 2, 4, 8, 10, 12]
    # for thread in threads:
    #     categories = DATA_SPEC_THREADS(thread)

    #     gen_generate_shared_plots(
    #         thread,
    #         result_mapped,
    #         0.20,
    #         out_dir,
    #         ["Baseline", *interesting_labels],
    #         categories,
    #         f"End-to-end Time for DuckDB (SF={sf})",
    #         "Total end-to-end time (s)",
    #         value_data_key="phase_total_time_median",
    #         value_median_key="phase_total_time_stdev",
    #     )
    #     gen_generate_shared_plots(
    #         thread,
    #         result_mapped,
    #         0.20,
    #         out_dir,
    #         ["Baseline", *interesting_labels],
    #         categories,
    #         f"Phase 1 Time for DuckDB (SF={sf})",
    #         "Phase 1 Time (s)",
    #         value_data_key="phase_1_explain_time_median",
    #         value_median_key="phase_1_explain_time_stdev",
    #     )
    #     gen_generate_shared_plots(
    #         thread,
    #         result_mapped,
    #         0.15,
    #         out_dir,
    #         [*interesting_labels],
    #         categories[1:],
    #         f"End-to-end Slowdown for DuckDB (SF={sf})",
    #         "Total end-to-end time (s)",
    #         value_data_key="phase_all_slowdown",
    #         value_median_key=None,
    #         needs_extra=True,
    #     )

    scalability_cats = list(map(replace_thread, DATA_SPEC_THREADS(0)[1:-1]))
    scalability_cats = ["base", *scalability_cats]
    new_labels = ["Base", *interesting_labels]
    scalability = compute_scalability_ratio(result_mapped)
    plot_scalability_plot(scalability, out_dir, scalability_cats, new_labels)


def compute_scalability_ratio(all_results: list[tuple[tuple, dict]]):
    mapping = {
        (replace_thread(key[0]), key[1]): {
            query: query_data
            for query, query_data in value.items()
        }
        for (key, value) in all_results
        if key[1] not in (4, 10)
    }
    print(mapping.keys())
    scalability = dict()
    for (category, thread), query_data in mapping.items():
        # if category == "base":
        #     continue
        # no point in adding this.
        # if thread == 1:
        #     continue
        old_thread = scalability.get(thread, {})
        new_category_key = replace_thread(category)
        assert new_category_key not in old_thread
        scalability[thread] = {
            **old_thread,
            new_category_key: {
                query: query_data[query]
                # query: query_data[query] / mapping[(new_category_key, 1)][query]
                for query in query_data
            },
        }
    return scalability


def main():
    parsed, out_dir = setup_tpch_analyzer_parser(mpl)
    config = json_read_file(parsed.config, True)
    assert config is not None
    db_file = out_dir / "analyze.db"
    if not parsed.use_cache:
        in_dir = Path(parsed.dir)
        normalized_rows: list[NormalizedDuckTPCHRow] = []
        analyze = ResultAnalyzer()
        for key, item in config.items():
            for file in item["file"]:
                result_file_path: Path = in_dir / file / "result.json"
                assert (
                    result_file_path.exists()
                ), f"Expected {result_file_path} to exist!"
                _rows = getattr(analyze, item["handler"])(key, result_file_path)
                normalized_rows.extend(_rows)
        flatted = [row for group in normalized_rows for row in group.normalize()]
        flatted = flatted[::-1]
        path = just_write(out_dir / "flat.json", json.dumps(flatted))
        if isinstance(path, Path):
            path = path.as_posix()

        conn = duckdb.connect(db_file)
        cursor = conn.cursor()
        cursor.execute(
            f"create or replace table normalized as (select * from read_json_auto('{path}'))"
        )
        stats_sql = just_read("./query_stats.sql")
        cursor.execute(stats_sql)
        slowdown_sql = just_read("./query_slowdown.sql")
        cursor.execute(slowdown_sql)
        cursor.close()
        conn.close()
    gen_plots(db_file, out_dir, parsed.sf)


if __name__ == "__main__":
    main()
