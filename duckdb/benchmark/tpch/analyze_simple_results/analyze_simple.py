# the simpler handling case.

import argparse
import json
import os
from pathlib import Path
from typing import Tuple

from matplotlib import pyplot as plt
import matplotlib as mpl
import numpy as np

from traceprovpy.tools.duckdb_inference import DuckDBDriverOptions
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse
from traceprovpy.tools.file_utils import (
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
from traceprovpy.tools.plot_utils import BenchmarkPlot
from traceprovpy.tools.run_duckdb_generic import add_query_options
import duckdb


class NormalizedDuckTPCHRow(Normalizable):
    category: str
    query_num: str
    threads: int
    phase_1: Extendable
    phase_1_profile: Extendable
    phase_2: Extendable

    def keys(self):
        return {
            "category",
            "query_num",
            "phase_1",
            "phase_1_profile",
            "phase_2",
            "threads",
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
        for query_num, query_data in query_results.items():
            for category, category_data in query_data.items():
                category_name = f"{category}_{suffix}"
                if "timeout" in category_data:
                    print("Handing timeout!")
                    rows.append(
                        NormalizedDuckTPCHRow(
                            category=category_name,
                            query_num=query_num,
                            phase_1=Extendable([make_dummy_simple_result(-1)] * 15),
                            phase_1_profile=Extendable(
                                [make_dummy_profile_result(-1)] * 15
                            ),
                            phase_2=None,
                            threads=bench_parsed.threads,
                        )
                    )
                    continue
                elif "errorcode" in category_data:
                    rows.append(
                        NormalizedDuckTPCHRow(
                            category=category_name,
                            query_num=query_num,
                            phase_1=Extendable([make_dummy_simple_result(-2)] * 15),
                            phase_1_profile=Extendable(
                                [make_dummy_profile_result(-2)] * 15
                            ),
                            phase_2=None,
                            threads=bench_parsed.threads,
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
                )
            )
        all_rows = [*rows]
        base_key = ("base", bench_parsed.threads)
        if base_key not in self.added_base:
            all_rows.extend(base_rows)
        self.added_base.add(base_key)
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
                    label="Timeout (> 5 min)",
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
    "gprom_join_heuristics_optimized-n__threads-1",
    "gprom_join_heuristics_optimized-n__threads-2",
    "gprom_join_heuristics_optimized-n__threads-4",
    "gprom_join_heuristics_optimized-n__threads-8",
    "gprom_join_heuristics_optimized-n__threads-10",
    "gprom_join_heuristics_optimized-n__threads-12",
    "gprom_join_optimized-n__threads-1",
    "gprom_join_optimized-n__threads-2",
    "gprom_join_optimized-n__threads-4",
    "gprom_join_optimized-n__threads-8",
    "gprom_join_optimized-n__threads-10",
    "gprom_join_optimized-n__threads-12",
    "gprom_window_heuristics_optimized-n__threads-1",
    "gprom_window_heuristics_optimized-n__threads-2",
    "gprom_window_heuristics_optimized-n__threads-4",
    "gprom_window_heuristics_optimized-n__threads-8",
    "gprom_window_heuristics_optimized-n__threads-10",
    "gprom_window_heuristics_optimized-n__threads-12",
    "gprom_window_optimized-n__threads-1",
    "gprom_window_optimized-n__threads-2",
    "gprom_window_optimized-n__threads-4",
    "gprom_window_optimized-n__threads-8",
    "gprom_window_optimized-n__threads-10",
    "gprom_window_optimized-n__threads-12",
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
        "gprom_join_optimized-n__threads-1",
        "gprom_join_optimized-n__threads-2",
        "gprom_join_optimized-n__threads-4",
        "gprom_join_optimized-n__threads-8",
        "gprom_join_optimized-n__threads-10",
        "gprom_join_optimized-n__threads-12",
    ],
    "gprom_join_heuristics": [
        "gprom_join_heuristics_optimized-n__threads-1",
        "gprom_join_heuristics_optimized-n__threads-2",
        "gprom_join_heuristics_optimized-n__threads-4",
        "gprom_join_heuristics_optimized-n__threads-8",
        "gprom_join_heuristics_optimized-n__threads-10",
        "gprom_join_heuristics_optimized-n__threads-12",
    ],
    "gprom_window": [
        "gprom_window_optimized-n__threads-1",
        "gprom_window_optimized-n__threads-2",
        "gprom_window_optimized-n__threads-4",
        "gprom_window_optimized-n__threads-8",
        "gprom_window_optimized-n__threads-10",
        "gprom_window_optimized-n__threads-12",
    ],
    "gprom_window_heuristics": [
        "gprom_window_heuristics_optimized-n__threads-1",
        "gprom_window_heuristics_optimized-n__threads-2",
        "gprom_window_heuristics_optimized-n__threads-4",
        "gprom_window_heuristics_optimized-n__threads-8",
        "gprom_window_heuristics_optimized-n__threads-10",
        "gprom_window_heuristics_optimized-n__threads-12",
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
            if isinstance(item, str):
                filtered = [x for x in original_data if x[0][0] == item][0]
            else:
                print(item)
                for x in original_data:
                    print(x[0])
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
    f"gprom_join_optimized-n__threads-{threads_count}",
    f"gprom_join_heuristics_optimized-n__threads-{threads_count}",
    f"gprom_window_optimized-n__threads-{threads_count}",
    f"gprom_window_heuristics_optimized-n__threads-{threads_count}",
    f"traceprov_optimized-y__threads-{threads_count}__compact-y__merge_chunks-y__table_stats-y",
]


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
    value_data_key,
    value_median_key,
    needs_extra=False,
):
    x_axis_values = list(map(str, range(1, 23)))
    x_axis = np.arange(len(x_axis_values))
    time_plot_fig, top_plot_axis = plt.subplots(
        1, 1, figsize=(14, 6), gridspec_kw={"hspace": 0.5, "wspace": 0}
    )
    data = []
    for item in categories:
        filtered = filter_data(original_data, item)
        data.append(filtered)
    timeout_max = 0
    pending_bars = []
    timeout_label = "Timeout (> 5 min)"
    errorout_label = "Error"
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
    top_plot_axis.set_title(
        f"End To End Comparison (threads {thread_count}) ({value_data_key.capitalize()})"
    )
    top_plot_axis.set_xticks(
        x_axis + width * (len(categories) / 2),
        x_axis_values,
    )
    legend_labels_all = []
    handles_all = []
    for _bar_idx, bar in enumerate(pending_bars):
        bar["height"] = timeout_max
        top_plot_axis.bar(**bar)
        handles, legend_labels = top_plot_axis.get_legend_handles_labels()
        legend_filtered = list(
            _idx
            for (_idx, ll) in enumerate(legend_labels)
            if (ll not in legend_labels_all)
        )
        handles_all.extend(handles[idx] for idx in legend_filtered)
        legend_labels_all.extend(legend_labels[idx] for idx in legend_filtered)
    top_plot_axis.legend(
        handles=handles_all,
        labels=legend_labels_all,
        loc="center right",
        bbox_to_anchor=(1.10, 0.95),
    )
    time_plot_fig.savefig(
        out_dir / f"end_to_end_comparison_{thread_count}_{value_data_key}.pdf"
    )


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
    gen_time_plots(
        result_mapped,
        GPROM_CASES,
        "gprom_all",
        sf,
        out_dir,
        0.1,
        [
            "(Thread 1)",
            "(Thread 2)",
            "(Thread 4)",
            "(Thread 8)",
            "(Thread 10)",
            "(Thread 12)",
        ],
        value_data_key="phase_1_explain_time",
    )
    gen_time_plots(
        result_mapped,
        TRACEPROV_OPTIMIZED_CASES,
        "traceprov_capture",
        sf,
        out_dir,
        0.1,
        [
            "(Thread 1)",
            "(Thread 2)",
            "(Thread 4)",
            "(Thread 8)",
            "(Thread 10)",
            "(Thread 12)",
        ],
        value_data_key="phase_1_explain_time",
    )
    gen_time_plots(
        result_mapped,
        TRACEPROV_OPTIMIZED_CASES,
        "traceprov_end_to_end",
        sf,
        out_dir,
        0.1,
        [
            "(Thread 1)",
            "(Thread 2)",
            "(Thread 4)",
            "(Thread 8)",
            "(Thread 10)",
            "(Thread 12)",
        ],
        value_data_key="phase_total_time",
    )
    interesting_labels = [
        "GProM Join",
        "GProM Join Heu.",
        "GProM Window",
        "GProM Window Heu.",
        "TraceProv Stats",
    ]
    threads = [1, 2, 4, 8, 10, 12]
    for thread in threads:
        categories = DATA_SPEC_THREADS(thread)
        gen_generate_shared_plots(
            thread,
            result_mapped,
            0.15,
            out_dir,
            ["base", *interesting_labels],
            categories,
            value_data_key="phase_total_time_median",
            value_median_key="phase_total_time_stdev",
        )
        gen_generate_shared_plots(
            thread,
            result_mapped,
            0.15,
            out_dir,
            [*interesting_labels],
            categories[1:],
            value_data_key="phase_all_slowdown",
            value_median_key=None,
            needs_extra=True,
        )


def main():
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
    config = json_read_file(parsed.config, True)
    out_dir = Path(parsed.out_dir) / parsed.sf
    os.makedirs(out_dir, exist_ok=True)
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
