import argparse
from functools import reduce
from itertools import product
import json
import math
import os
from pathlib import Path
import re
import statistics
import sys
from typing import Callable, Dict, Literal, NamedTuple

from matplotlib import pyplot as plt, ticker
import numpy as np

from traceprovpy.tools.duckdb_inference import DuckDBDriverOptions
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse
from traceprovpy.tools.file_utils import just_read, just_write
from traceprovpy.tools.normalized_row import (
    Extendable,
    Normalizable,
    NormalizedRow,
    NormalizedSampleInferRow,
    sum_simple_result,
    tap_profile_result,
    tap_simple_result,
)
from traceprovpy.tools.plot_utils import BenchmarkPlot
from traceprovpy.tools.run_duckdb_generic import (
    TRACEPROV_CAPTURE_ENTRY,
    TRACEPROV_CAPTURE_ENTRY_SD,
    add_query_options,
)

import duckdb

import matplotlib as mpl
from matplotlib.patches import Patch

import random

from traceprovpy.tools.zipf import zipf_draw
import pandas as pd

# mpl.rcParams.update(
#     {
#         # fonts
#         "font.family": "serif",
#         "font.size": 16,
#         "axes.labelsize": 18,
#         "xtick.labelsize": 16,
#         "ytick.labelsize": 16,
#         "legend.fontsize": 15,
#         "axes.titlesize": 18,
#         # cleaner look
#         "axes.spines.top": False,
#         "axes.spines.right": False,
#         # lines
#         "lines.linewidth": 2,
#         "patch.linewidth": 1.5,
#     }
# )


class TpchSampleRow(Normalizable):
    category: str
    query_num: str
    latency: float
    offset: int
    iter_id: int
    layer_number: int
    index_build_time: float
    sql_time: float
    partition_time: float

    def keys(self):
        return {
            "category",
            "latency",
            "offset",
            "iter_id",
            "layer_number",
            "query_num",
            "index_build_time",
            "sql_time",
            "partition_time",
        }

    def types(self):
        return TpchSampleRow(
            category=str,
            query_num=str,
            latency=float,
            offset=int,
            iter_id=int,
            layer_number=int,
            index_build_time=float,
            sql_time=float,
            partition_time=float,
        )


PY_DUCKDB_MAPPING = {str: "VARCHAR", float: "double", int: "bigint"}


class TpchRow(NormalizedSampleInferRow):
    query_num: str
    # dict because there are multiple types of this.
    log_sizes: Extendable
    extra: Extendable

    def keys(self):
        return super().keys() | {"query_num", "log_sizes", "extra"}


# basically flattens keys into individual values.
def _reduce_keys(prev: dict, curr: dict):
    return {
        **prev,
        **{key: [*prev.get(key, []), value] for (key, value) in curr.items()},
    }


def remove_throwaway(in_values: list, throwaway_value: int):
    return in_values[throwaway_value:]


def aggregate_result(results: list[dict], throwaway_value: int):
    flattened = reduce(_reduce_keys, results, dict())
    median_values = {
        f"{key}_median": statistics.median(remove_throwaway(values, throwaway_value))
        for (key, values) in flattened.items()
    }
    stdev_ratio_values = {
        f"{key}_std_ratio": statistics.stdev(remove_throwaway(values, throwaway_value))
        / statistics.mean(remove_throwaway(values, throwaway_value))
        for (key, values) in flattened.items()
    }
    std_values = {
        f"{key}_std": statistics.stdev(remove_throwaway(values, throwaway_value))
        for (key, values) in flattened.items()
    }
    agg = {**median_values, **stdev_ratio_values, **std_values}
    return agg


def aggregate_over_samples(results: list[dict]):
    return dict(
        average_time=statistics.mean(result["latency_median"] for result in results),
        min_backtrace_time=min(result["latency_median"] for result in results),
        max_backtrace_time=max(result["latency_median"] for result in results),
        std_backtrace_time=(
            None
            if len(results) == 1
            else statistics.stdev(result["latency_median"] for result in results)
        ),
        max_stdev_ratio=max(result["latency_std_ratio"] for result in results),
        max_stdev=max(result["latency_std"] for result in results),
        mean_stdev=math.sqrt(
            sum(result["latency_std"] ** 2 for result in results) / len(results)
        ),
        sum_backtrace_time=sum(result["latency_median"] for result in results),
    )


def get_sd_base(result_item: dict):
    return result_item["sd"]


def extract_str(in_regex):
    def _extract(in_str: int):
        matches = re.findall(in_regex, in_str)
        if len(matches) == 0:
            return 0
        assert len(matches) == 1
        return int(matches[0])

    return _extract


EXTRACT_LAYER = extract_str(r"layer-(\d+)")
EXTRACT_PARTITION = extract_str(r"partition_time-(\d+)")
EXTRACT_INDEX = extract_str(r"partition_time-(\d+)")


def analyze_result_item(
    result_item: dict, throwaway_value: int, base_getter: Callable[[Dict], Dict] = None
):
    print("Current getter: ", base_getter)
    base_result_item = result_item["result"]
    if base_getter:
        base_result_item = base_getter(base_result_item)
    base_result = base_result_item["base_profile"]
    sample_infer_result = result_item["sample_inference_result"]
    capture_time_items = None
    capture_profile_items = None
    log_sizes = None
    sample_result_rows = []
    index_build_time = 0.0
    if "return_code" not in sample_infer_result:
        sql_spec_map = sample_infer_result["sql_spec_map"]
        capture_indexes = [
            l_idx
            for (l_idx, (map_entry, _)) in enumerate(sql_spec_map)
            if tuple(map_entry) in (TRACEPROV_CAPTURE_ENTRY, TRACEPROV_CAPTURE_ENTRY_SD)
        ]
        # a fine assumption (as a sanity check.)
        assert len(capture_indexes) == len(base_result)

        sample_result = dict()
        misc_sample_extra = dict()
        last_capture_result = None
        result_added = set()
        for profile_entry_idx, profile_entry in enumerate(
            sample_infer_result["profile"]
        ):
            if profile_entry_idx in capture_indexes:
                last_capture_result = sample_infer_result["result_time"][
                    profile_entry_idx
                ]
                continue
            first_key, iter_id = sql_spec_map[profile_entry_idx]
            composite_key = (tuple(first_key), iter_id)
            if composite_key in result_added:
                continue
            result_added.add(composite_key)
            # print(profile_entry)
            assert len(first_key) in (2, 3)
            if len(first_key) == 2:
                part_key = first_key
            else:
                part_key = first_key[1:]

            # print("using keys: ", part_key, first_key)
            part_key = tuple(part_key)
            post_process_time = 0
            if part_key not in sample_result:
                sample_result[part_key] = [dict() for _ in range(len(capture_indexes))]
                misc_sample_extra[part_key] = [
                    dict() for _ in range(len(capture_indexes))
                ]
            # For SmokedDuck
            layer_number = 0
            extra_stats = None
            if not base_getter:
                extra_item = sample_infer_result["result_time"][profile_entry_idx][
                    "option"
                ]["extra"][0]
                layer_number = EXTRACT_LAYER(extra_item)
                partition_time = EXTRACT_PARTITION(extra_item) / 10**6
                # print(last_capture_result["option"], extra_item)
                # sql_time = 0
                sql_time = last_capture_result["option"].get(
                    "misc_key_value_sql_compilation_time", 0
                )
                index_build_time = last_capture_result["option"].get(
                    "misc_key_value_index_time", 0
                ) / (10**6)
                # Need to include the post-process time just once per offset.
                # This is very very unfair to our system
                # Since we let SmokedDuck get away with doing post process per query, but we're still faster ;)
                extra_stats = dict(
                    sql_time=sql_time / (10**6),
                    index_build_time=index_build_time,
                    partition_time=partition_time,
                )
            else:
                try:
                    stats = sample_infer_result["stats"][0]
                except KeyError:
                    stats = base_result_item["capture_stats"][0]
                if stats:
                    index_build_time = float(stats["build_time"])
                extra_stats = dict(
                    index_build_time=index_build_time,
                    sql_time=None,
                    partition_time=None,
                )
            sample_result[part_key][iter_id][layer_number] = tap_profile_result(
                profile_entry
            )
            misc_sample_extra[part_key][iter_id][layer_number] = extra_stats

        sample_result_rows = [
            dict(
                offset=offset[-1],
                iter_id=iter_id,
                layer_number=layer_number,
                **layer_result,
                **misc_sample_extra[offset][iter_id][layer_number],
            )
            for offset, offset_result in sample_result.items()
            for iter_id, iter_result in enumerate(offset_result)
            for layer_number, layer_result in (iter_result.items())
        ]

        # need to reduce the result up a bit
        sample_result_reduced = aggregate_over_samples(
            [
                aggregate_result(
                    [
                        reduce(sum_simple_result, list(iter_result.values()))
                        for iter_result in sample_results
                    ],
                    throwaway_value=throwaway_value,
                )
                for sample_results in (sample_result.values())
            ]
        )
        capture_time_items = [
            sample_infer_result["result_time"][l_idx] for l_idx in capture_indexes
        ]
        capture_profile_items = [
            sample_infer_result["profile"][l_idx] for l_idx in capture_indexes
        ]
    else:
        # eh. This will be the case for SmokedDuck.
        # Unfortunatalely, need to some more massaging to that Smoked Duck result, and make it
        # more easy to digest :)
        assert base_getter is not None
        sample_result_reduced = dict(
            average_time=None,
            max_stdev_ratio=None,
            max_stdev=None,
            mean_stdev=None,
            min_backtrace_time=None,
            max_backtrace_time=None,
            std_backtrace_time=None,
            sum_backtrace_time=None,
        )
        capture_time_items = base_result_item["capture_time"]
        capture_profile_items = base_result_item["capture_profile"]
        capture_indexes = None

    if base_getter is not None:
        # need to map the log sizes too.
        stats = None
        try:
            stats = sample_infer_result["stats"][0]
        except KeyError:
            stats = base_result_item["capture_stats"][0]
        if stats:
            index_build_time = float(stats["build_time"])
            log_sizes = (
                (
                    [
                        dict(
                            page_requested_size=stats["size_mb"],
                            page_used_size=stats["size_mb"],
                            bytes_used_size=stats["size_mb"],
                        )
                    ]
                    * len(capture_indexes)
                )
                if capture_indexes
                else None
            )
            extras = (
                (
                    [dict(index_build_time=index_build_time, part_time=0)]
                    * len(capture_indexes)
                )
                if capture_indexes
                else None
            )

        # try:
        #     index_building_time = sample_infer_result["stats"][0]["build_time"]
        # except KeyError:
        #     index_building_time = 0

    else:
        log_sizes = [
            capture_time["option"]["misc_key_value_total_log_size"][0]
            for capture_time in capture_time_items
        ]
        extras = [
            dict(index_build_time=index_build_time, part_time=0)
            for _ in capture_time_items
        ]

    base_and_capture = dict(
        base=Extendable(map(tap_simple_result, base_result_item["base_time"])),
        base_profile=Extendable(
            map(tap_profile_result, base_result_item["base_profile"])
        ),
        capture=(
            None
            if capture_time_items is None
            # TODO: Maybe handle the case when extendables are 0?
            else Extendable(map(tap_simple_result, capture_time_items))
        ),
        capture_profile=(
            None
            if capture_profile_items is None
            else Extendable(map(tap_profile_result, capture_profile_items))
        ),
        log_sizes=Extendable(log_sizes) if log_sizes is not None else None,
        extra=Extendable(extras) if extras is not None else None,
    )

    return {**base_and_capture, **sample_result_reduced}, sample_result_rows


def parse_category(category: str):
    if category == "SmokedDuck":
        return category
    cleaned = category.replace("optimized-y__threads-1", "").strip("__")
    option_split = cleaned.split("__")
    options = []
    for option in option_split:
        splitted = option.split("-")
        if len(splitted) != 2:
            continue
        option_name, option_value = splitted
        options.append(f"{option_name[0].capitalize()}-{option_value}")
    joined = ",".join(options)
    return f"TraceProv ({joined})"


def null_safe(in_list: list):
    return [0 if i is None else i for i in in_list]


ALL_CATEGORIES = [
    "SmokedDuck",
    "optimized-y__threads-1",
    "optimized-y__threads-1__compact-y",
    "optimized-y__threads-1__compact-y__merge_chunks-y",
    "optimized-y__threads-1__compact-y__merge_chunks-y__partition_in_agg-y",
    "optimized-y__threads-1__compact-y__merge_chunks-y__partition_in_agg-y__table_stats-y",
    "optimized-y__threads-1__compact-y__merge_chunks-y__table_stats-y",
    "optimized-y__threads-1__compact-y__partition_in_agg-y",
    "optimized-y__threads-1__compact-y__partition_in_agg-y__table_stats-y",
    "optimized-y__threads-1__compact-y__table_stats-y",
    "optimized-y__threads-1__merge_chunks-y",
    "optimized-y__threads-1__merge_chunks-y__partition_in_agg-y",
    "optimized-y__threads-1__merge_chunks-y__partition_in_agg-y__table_stats-y",
    "optimized-y__threads-1__merge_chunks-y__table_stats-y",
    "optimized-y__threads-1__partition_in_agg-y",
    "optimized-y__threads-1__partition_in_agg-y__table_stats-y",
    "optimized-y__threads-1__table_stats-y",
]

CATEGORIES = [
    "SmokedDuck",
    "optimized-y__threads-1",
    "optimized-y__threads-1__compact-y",
    "optimized-y__threads-1__merge_chunks-y",
    "optimized-y__threads-1__compact-y__merge_chunks-y",
    "optimized-y__threads-1__partition_in_agg-y",
    "optimized-y__threads-1__compact-y__partition_in_agg-y",
    "optimized-y__threads-1__merge_chunks-y__partition_in_agg-y",
    "optimized-y__threads-1__compact-y__merge_chunks-y__partition_in_agg-y",
]

INTERESTING_CATEGORIES = [
    "SmokedDuck",
    "optimized-n__threads-1__partition_in_agg-y",
    "optimized-n__threads-1__join_filter_rewrite-y__partition_in_agg-y",
    "optimized-n__threads-1__filter_pushdown-y__join_filter_rewrite-y__partition_in_agg-y",
    "optimized-n__threads-1__index-y",
    # "optimized-n__threads-1",
    # "optimized-n__threads-1__index-y",
    # # "optimized-y__threads-1__compact-y__merge_chunks-y",
    # "optimized-y__threads-1__compact-y__merge_chunks-y__table_stats-y",
    # "optimized-y__threads-1__compact-y__table_stats-y",
    # # "optimized-y__threads-1__compact-y__merge_chunks-y__partition_in_agg-y",
    # "optimized-y__threads-1__compact-y__merge_chunks-y__partition_in_agg-y__table_stats-y",
]

INTERESTING_CATEGORIES_MAP = {
    "optimized-y__threads-1__compact-y__merge_chunks-y__table_stats-y": "TraceProv",
    "optimized-y__threads-1__compact-y__table_stats-y": "TraceProv (w/o merge)",
    "optimized-y__threads-1__compact-y__merge_chunks-y__partition_in_agg-y__table_stats-y": "TraceProv (Partition)",
    "optimized-n__threads-1__index-y": "TraceProv (index)",
    "optimized-n__threads-1": "TraceProv (no index)",
}

LOG_SIZE_CATEGORY = [
    "SmokedDuck",
    "optimized-n__threads-1__index-y",
    # "optimized-y__threads-1",
    # "optimized-y__threads-1__compact-y",
    # "optimized-y__threads-1__partition_in_agg-y",
    # "optimized-y__threads-1__compact-y__partition_in_agg-y",
]


def query_layer_mapping():
    return {"2": [3], "4": [2], "17": [3], "18": [3], "20": [5], "21": [2], "22": [3]}


def run_query(cursor, query):
    cursor.execute(query)
    return cursor.fetchall()


def nice_str(in_float: float):
    if in_float < 100:
        return str(in_float)
    if in_float < 1000:
        # Being fair to SmokedDuck here :)
        return str(int(math.floor(in_float)))
    shrunk = in_float / 1_000
    rounded = round(shrunk, 1)
    return f"{rounded} K"


def get_slowdown(_sd_time, _tp_time):
    slowdown = (
        nice_str(round(_sd_time / _tp_time, 1))
        if _sd_time is not None and _tp_time is not None
        else "-"
    )
    return slowdown


MAX_REPEAT = 100


def get_nice_num(in_int):
    if in_int < 1_000:
        return str(int(in_int))
    if in_int < 1_000_000:
        return f"{int(in_int / 1_000)} K"
    if in_int < 1_000_000_000:
        return f"{int(in_int / 1_000_000)} M"
    if in_int < 1_000_000_000_000:
        return f"{int(in_int / 1_000_000_000)} B"
    assert 0, in_int


def plot_repeat(
    cursor, sf: str, out_dir: Path, fetched_results, categories, label: str
):
    fetched_results_map = {
        category: {
            item["query_num"]: item["capture_profile_latency"]
            for item in category_result
        }
        for (category, category_result) in fetched_results
    }
    query_row_count_result = run_query(
        cursor,
        "select query_num,any_value(base_row_count) from dumped group by query_num;",
    )
    query_row_count_result_map = {
        query_num: count for (query_num, count) in query_row_count_result
    }
    percents = [1, 5, 10, 20, 50, 100]
    # percents = [50]
    sample_offset_table = "sample_offset"
    percent_result = dict()
    offset_query = just_read("get_offset.sql")
    random.seed(10)
    queries = [q for q in list(map(str, range(1, 23))) if q not in ["16"]]
    # queries = ["1"]
    multiplier = [1, 10, 1000, 10000, 1_000_000]
    multiplier_label = {1: "1", 10: "10", 1000: "1K", 10000: "10K", 1_000_000: "1M"}
    raw_percent_results = dict()
    for percent in percents:
        print("Running: ", percent)
        cursor.execute(f"drop table if exists {sample_offset_table}")
        cursor.execute(
            f"create table {sample_offset_table} (query_num varchar, experiment bigint, i_offset bigint)"
        )
        dict_values = dict(query_num=[], experiment=[], i_offset=[])
        for query, query_base_count in sorted(
            query_row_count_result_map.items(), key=lambda x: x[0]
        ):
            if query not in queries:
                continue
            offsets = range(query_base_count)
            print(query_base_count)
            sample_count = int(math.ceil(percent * query_base_count / 100))
            random.seed(10)
            all_samples = []
            for _ in range(1000):
                rand_sample = random.choices(offsets, k=sample_count)
                all_samples.append(list(rand_sample))
            # if query == "1":
            #     print("Sampling --> ", query, sampled, query_base_count)

            for exp_id, samples in enumerate(all_samples):
                for sample in samples:
                    dict_values["experiment"].append(exp_id)
                    dict_values["query_num"].append(str(query))
                    dict_values["i_offset"].append(sample)

        df_data = pd.DataFrame(dict_values)
        cursor.execute(
            f"insert into {sample_offset_table} BY NAME select * from df_data"
        )
        category_results = run_query(cursor, offset_query)
        raw_percent_results[percent] = category_results
        for _mult in multiplier:
            percent_result[(_mult, percent)] = {
                category: {
                    result_item["query_num"]: {
                        "total_time": _mult * result_item["latency"]
                        + (result_item["index_build_time"] or 0),
                        "base_time": fetched_results_map[category][
                            result_item["query_num"]
                        ],
                        "repeat_count": result_item["repeat_count"],
                    }
                    for result_item in category_result
                }
                for (category, category_result) in category_results
            }
    # print(percent_result)
    all_percent_result = []
    all_percent_result_total = []

    # mapping_labels = []
    percent_result_mapping = product(multiplier, percents)
    # percent_result_mapping = [(1, 1), (1, 5), (1, 10), (1, 20), (1, 50), (1, 100)]
    for key in percent_result_mapping:
        percent_multiplier, percent = key
        _percent_result = percent_result[key]
        traceprov_time = _percent_result[categories[1]]
        sd_time = _percent_result[categories[0]]
        print(key)
        print("TraceProv Time: ")
        print(traceprov_time)
        print("SD time")
        print(sd_time)
        query_results = [f"{multiplier_label[percent_multiplier]}x{percent}%"]
        query_total_results = [f"{multiplier_label[percent_multiplier]}x{percent}%"]
        for query in queries:
            if query in traceprov_time:
                _tp_time = traceprov_time[query]["total_time"]
                _tp_total_time = _tp_time + traceprov_time[query]["base_time"]
                _tp_repeat_count = traceprov_time[query]["repeat_count"]
            else:
                _tp_time = None
                _tp_total_time = None
                _tp_repeat_count = None
            if query in sd_time:
                _sd_time = sd_time[query]["total_time"]
                _sd_total_time = _sd_time + sd_time[query]["base_time"]
                _sd_repeat_count = sd_time[query]["repeat_count"]
            else:
                _sd_time = None
                _sd_total_time = None
                _sd_repeat_count = None

            if _sd_repeat_count != _tp_repeat_count:
                print(
                    "Got mismatching counts: ",
                    _sd_repeat_count,
                    _tp_repeat_count,
                    query,
                )
            slowdown = get_slowdown(_sd_time, _tp_time)
            total_slowdown = get_slowdown(_sd_total_time, _tp_total_time)

            # print(percent, query, _tp_time, _sd_time, slowdown, _sd_time / _tp_time)
            if slowdown == "-":
                print(percent, query, _tp_time, _sd_time)
            query_results.append(
                f"{slowdown}\n{get_nice_num(percent_multiplier*_sd_repeat_count)}"
            )
            query_total_results.append(
                f"{total_slowdown}\n{get_nice_num(percent_multiplier*_sd_repeat_count)}"
            )
        # print(len(query_results))
        all_percent_result.append(query_results)
        all_percent_result_total.append(query_total_results)

    col_labels = ["repeat", *queries]
    missing_columns = [
        col_labels.index(query)
        for query, count in query_row_count_result_map.items()
        if count > 100 and query in queries
    ]

    fig, ax = plt.subplots(figsize=(8, 2))
    ax.axis("off")  # Hide the plot axes completely

    print(len(col_labels))
    # Add the table to the axis
    table = ax.table(
        cellText=all_percent_result,
        colLabels=col_labels,
        loc="center",
        cellLoc="center",
    )
    table.auto_set_font_size(False)
    table.set_fontsize(11)

    # column widths — auto fit to content
    table.auto_set_column_width(col=list(range(0, len(col_labels))))
    for (row, col), cell in table.get_celld().items():
        cell.set_linewidth(0.85)
    table.scale(1, 1.9)
    for cell in table.get_celld().values():
        # print(cell.PAD)
        cell.PAD = 0.05  # default is around 0.1
    # table.scale(1.4, 0.85)
    cells = table.get_celld()
    for column in missing_columns:
        cells[(0, column)].set_facecolor("lightgreen")
    fig.savefig(out_dir / f"iter_count.pdf", bbox_inches="tight")

    fig, ax = plt.subplots(figsize=(8, 2))
    ax.axis("off")  # Hide the plot axes completely

    col_labels = ["%", *queries]
    print(len(col_labels))
    # Add the table to the axis
    table = ax.table(
        cellText=all_percent_result_total,
        colLabels=col_labels,
        loc="center",
        cellLoc="center",
    )
    # Loop through all cells to change line thickness
    for (row, col), cell in table.get_celld().items():
        cell.set_linewidth(0.85)
    table.set_fontsize(15)
    for cell in table.get_celld().values():
        # print(cell.PAD)
        cell.PAD = 0.05  # default is around 0.1
    table.scale(1.4, 0.85)
    cells = table.get_celld()
    for column in missing_columns:
        cells[(0, column)].set_facecolor("lightgreen")
    fig.savefig(
        out_dir / f"iter_count_total.pdf",
        bbox_inches="tight",
    )
    plot_overall_breakeven(
        raw_percent_results, categories, query_row_count_result_map, out_dir
    )


def get_percent_break(sd_percent_result: dict, current_percent_result: dict):
    assert (sd_percent_result.keys()) == (current_percent_result.keys())
    return {
        key: {
            query_num: _get_break(
                dict(
                    diff_index=sd_percent_result[key][query_num]["index_time"]
                    - current_percent_result[key][query_num]["index_time"],
                    diff_latency=current_percent_result[key][query_num]["latency"]
                    - sd_percent_result[key][query_num]["latency"],
                    record_count=current_percent_result[key][query_num]["repeat"],
                )
            )
            for query_num in sd_percent_result[key]
        }
        for key in sd_percent_result
    }


# returns False if latter never underforms
# returns the count otherwise
def _get_break(in_dict: dict):
    latency_diff = in_dict["diff_latency"]
    index_diff = in_dict["diff_index"]
    is_less = latency_diff < 0
    count = math.ceil(index_diff / latency_diff)
    if count < 0:
        if is_less:
            return False
        count = 0
    return dict(
        iter=count, is_less=is_less, total_count=int(in_dict["record_count"] * count)
    )


def plot_overall_breakeven(
    raw_percent_results: dict,
    categories: list[str],
    query_row_count_map: dict,
    out_dir: Path = None,
):
    mapped_result = {
        percent: {
            category: {
                result_item["query_num"]: dict(
                    repeat=result_item["repeat_count"],
                    latency=result_item["latency"],
                    index_time=(result_item["index_build_time"] or 0),
                )
                for result_item in category_result
            }
            for (category, category_result) in category_results
        }
        for (percent, category_results) in raw_percent_results.items()
    }
    all_categories = list(mapped_result.values())[0].keys()
    category_flipped = {
        category: {
            percent: percent_result[category]
            for (percent, percent_result) in mapped_result.items()
        }
        for category in all_categories
    }
    sd_result = category_flipped[categories[0]]
    percent_diff_results = {
        category: get_percent_break(sd_result, category_result)
        for (category, category_result) in category_flipped.items()
        if category in categories[1:]
    }
    percent_diff_results_filtered = {
        category: {
            percent: {
                query_num: query_result
                for query_num, query_result in percent_result.items()
                if query_result != False
                and ((not query_result["is_less"]) or query_result["iter"] > 0)
            }
            for percent, percent_result in category_result.items()
        }
        for category, category_result in percent_diff_results.items()
    }
    # total_time_fig, total_time_axis = plt.subplots(1, 1, figsize=figsize)
    #
    just_write("./tmp/percent_diff.json", json.dumps(percent_diff_results_filtered))
    # figure out the smallest subsection of queries that the results are available for.

    _query_num_available = [
        set(percent_result.keys())
        for _, category_result in percent_diff_results_filtered.items()
        for __, percent_result in category_result.items()
    ]
    query_available = reduce(
        lambda prev, curr: prev | curr, _query_num_available, set()
    )
    x_axis_values = sorted(list(query_available), key=lambda q: int(q))
    x_axis = np.arange(len(x_axis_values))
    percent_breakeven_fig, percent_breakeven_axis = plt.subplots(1, 1, figsize=(10, 4))
    width = 0.14
    group_gap = 0.01
    for category, category_result in percent_diff_results_filtered.items():
        category_result_sorted = sorted(
            category_result.items(), key=lambda per: int(per[0])
        )
        for percent_idx, (percent, percent_result) in enumerate(category_result_sorted):
            get_less = lambda key, is_less: [
                (
                    0
                    if x not in percent_result
                    else (
                        percent_result[x][key]
                        if not (is_less ^ percent_result[x]["is_less"])
                        else 0
                    )
                )
                for x in x_axis_values
            ]
            y_values = get_less("iter", False)
            y_values_less = get_less("iter", True)
            y_values_count = get_less("total_count", False)
            y_values_count_less = get_less("total_count", True)
            percent_breakeven_axis.bar(
                x_axis + width * percent_idx,
                y_values,
                width=width,
                label=f"{percent}%",
                color=BenchmarkPlot.colors[percent_idx],
            )
            percent_breakeven_axis.bar(
                x_axis + width * percent_idx,
                y_values_less,
                width=width,
                hatch="///",
                alpha=0.99,
                color=BenchmarkPlot.colors[percent_idx],
            )
            percent_breakeven_axis.bar(
                x_axis + width * percent_idx,
                [-1 * int(y) for y in y_values_count],
                width=width,
                # label=f"{percent}%",
                color=BenchmarkPlot.colors[percent_idx],
            )
            print(y_values_less)
            print(y_values_count)
            print(y_values_count_less)
            percent_breakeven_axis.bar(
                x_axis + width * percent_idx,
                [-1 * int(y) for y in y_values_count_less],
                width=width,
                hatch="///",
                color=BenchmarkPlot.colors[percent_idx],
                alpha=0.99,
            )
        # add row count bars
        for query_id, query in enumerate(x_axis_values):
            base_count = query_row_count_map[query]
            # x_start =
            percent_breakeven_axis.hlines(
                y=-1 * base_count,
                color="black",
                linestyle="solid",
                xmin=query_id - (width / 2),
                xmax=query_id + ((percent_idx + 1) * width) - (width / 2),
                linewidth=1,
            )
    percent_breakeven_axis.set_xticks(x_axis + width * (percent_idx / 2), x_axis_values)
    percent_breakeven_axis.set_yscale("symlog")
    yticks = []
    ytick_labels = []
    percent_breakeven_axis.axhline(y=0, color="black", linestyle="solid")
    percent_breakeven_axis.axhline(y=1, color="black", linestyle="--", linewidth=0.7)
    percent_breakeven_axis.set_xlabel("Query")
    percent_breakeven_axis.legend(ncols=3)
    percent_breakeven_axis.text(
        -0.05,
        0.75,
        "Breakeven \n Iteration",
        transform=percent_breakeven_axis.transAxes,
        ha="right",
        va="bottom",
        fontsize=10,
        horizontalalignment="center",
        verticalalignment="center",
        ma="center",
    )
    percent_breakeven_axis.text(
        -0.05,
        0.25,
        "Total Request \n Count",
        transform=percent_breakeven_axis.transAxes,
        ha="right",
        va="top",
        fontsize=10,
        horizontalalignment="center",
        verticalalignment="center",
        ma="center",
    )
    percent_breakeven_axis.set_ylabel(
        ""
    )  # leave main label empty, or use a generic axis title
    for tick_labels in percent_breakeven_axis.get_yticklabels():
        yticks.append(tick_labels.get_position()[1])
        ytick_labels.append(tick_labels.get_text().replace("-", ""))
    print(yticks, ytick_labels)
    percent_breakeven_axis.set_yticks(yticks, ytick_labels)
    percent_breakeven_axis.yaxis.set_minor_locator(
        ticker.SymmetricalLogLocator(
            base=10.0,
            subs=[2, 3, 4, 5, 6, 7, 8, 9],
            linthresh=percent_breakeven_axis.yaxis.get_transform().linthresh,
        )
    )
    percent_breakeven_axis.tick_params(axis="y", which="minor", left=True)

    percent_breakeven_axis.set_title(
        "Breakeven Iteration & \n Total Request Count per Query"
    )
    # percent_breakeven_axis.set_xticklabels(x_axis)
    percent_breakeven_fig.savefig(
        out_dir / "percent_breakeven.pdf", bbox_inches="tight"
    )


def plot_box_plot(cursor, sf: str, out_dir: Path):
    query = just_read("get_index_usage.sql")
    cursor.execute(query)
    results = cursor.fetchall()
    result_sorted = sorted(
        [(int(result[0]), result[1]) for result in results], key=lambda x: x[0]
    )
    query_list = [x[0] for x in result_sorted]
    boxplot_values = [x[1] for x in result_sorted]
    fig, axs = plt.subplots(figsize=(9, 4))
    axs.set_title("Breakeven Repetition")
    axs.boxplot(boxplot_values, whis=(0, 100), showfliers=False)
    # axs.set_ylim((, 2000))
    axs.set_yscale("symlog")
    axs.axhline(y=0, color="r", linestyle="--")
    axs.set_xticks(range(1, len(query_list) + 1), query_list)
    fig.savefig(out_dir / "breakeven_distribution.pdf")


def plot_result(
    fetched_result: list[tuple[str, list]],
    sf: str,
    categories: list[str],
    figsize=(20, 6),
    width=0.1,
    mapping: dict = None,
):
    x_axis_values = list(map(str, range(1, 23)))
    x_axis = np.arange(len(x_axis_values))
    log_size_width = 0.15
    fig, (fig_axis, infer_time_axis, infer_time_with_index_axis) = plt.subplots(
        3, 1, figsize=figsize
    )
    total_time_fig, total_time_axis = plt.subplots(1, 1, figsize=figsize)
    log_size_fig, log_size_axis = plt.subplots(1, 3, figsize=(25, 6), sharey=True)
    stdev_fig, (stdev_base_axis, stdev_capture_axis) = plt.subplots(
        2, 1, figsize=(20, 6), sharex=True
    )
    fetched_result = [item for item in fetched_result if item[0] in categories]
    result_sorted = sorted(fetched_result, key=lambda x: categories.index(x[0]))
    log_size_incr = 0
    fig_axis.axhline(y=10, color="r", linestyle="--")
    fig_axis.axhline(y=20, color="r", linestyle="--")
    fig_axis.set_yscale("log")
    extra_labels = dict()
    pseudo_idx = 0
    for category_idx, (category, category_data) in enumerate(result_sorted):
        remapped_data = {item["query_num"]: item for item in category_data}
        parsed_category = parse_category(category)
        x_axis_adjusted = x_axis + width * pseudo_idx
        x_axis_log_adjusted = x_axis + log_size_width * log_size_incr

        def _get_key_in_dict(in_key: str):
            return null_safe(
                [
                    remapped_data[q][in_key] if q in remapped_data else 0
                    for q in x_axis_values
                ]
            )

        def _get_not_present(in_key: str):
            return list(
                [
                    qidx
                    for qidx, q in enumerate(x_axis_values)
                    if q not in remapped_data or remapped_data[q][in_key] is None
                ]
            )

        y_values = _get_key_in_dict("relative_overhead")
        y_infer_time_values = _get_key_in_dict("average_time")
        y_infer_time_values_with_index = _get_key_in_dict("average_time_with_index")
        total_time_y_values = _get_key_in_dict("total_time")
        total_time_stdev = _get_key_in_dict("total_time_std")
        # y_infer_time_values_error = _get_key_in_dict("mean_stdev")
        y_infer_time_values_error = _get_key_in_dict("max_stdev")
        not_present = _get_not_present("average_time")
        print(not_present)
        # if category == "SmokedDuck":
        #     # y_values_absent = _get_not_present("relative_overhead")
        #     cap_value = 10 ** (-2)
        #     for qidx in not_present:
        #         x_pos = qidx + width * category_idx
        #         infer_time_axis.bar(
        #             x_pos,
        #             cap_value,
        #             width=width,
        #             color="lightgray",
        #             hatch="///",
        #             edgecolor="red",
        #         )
        #         infer_time_axis.annotate(
        #             "",
        #             xy=(x_pos, cap_value),
        #             ha="center",
        #             va="bottom",
        #             fontsize=10,
        #             color="red",
        #             fontweight="bold",
        #         )
        #     na_result = Patch(
        #         facecolor="lightgray",
        #         hatch="///",
        #         edgecolor="red",
        #         label="Not Available",
        #     )
        #     extra_labels["Not Available"] = na_result
        # else:
        # cap_value = 10 ** (-2)
        # for qidx in not_present:
        #     x_pos = qidx + width * category_idx
        #     infer_time_axis.bar(
        #         x_pos,
        #         cap_value,
        #         width=width,
        #         color="lightgray",
        #         hatch="...",
        #         edgecolor="green",
        #     )
        #     infer_time_axis.annotate(
        #         "",
        #         xy=(x_pos, cap_value),
        #         ha="center",
        #         va="bottom",
        #         fontsize=10,
        #         color="green",
        #         fontweight="bold",
        #     )
        # np_result = Patch(
        #     facecolor="lightgray",
        #     hatch="...",
        #     edgecolor="green",
        #     label="Not Applicable",
        # )
        # extra_labels["Not Applicable"] = np_result

        log_size_values = map(
            _get_key_in_dict,
            [
                "log_sizes_page_requested_size",
                "log_sizes_page_used_size",
                "log_sizes_bytes_used_size",
            ],
        )
        log_size_stdev = map(
            _get_key_in_dict,
            [
                "log_sizes_page_requested_size_stdev",
                "log_sizes_page_used_size_stdev",
                "log_sizes_bytes_used_size_stdev",
            ],
        )

        fig_axis.bar(
            x_axis_adjusted,
            y_values,
            width=width,
            label=(
                (mapping or dict()).get(
                    category,
                    (
                        f"{parsed_category} Capture"
                        if parsed_category != "SmokedDuck"
                        else "SmokedDuck"
                    ),
                )
            ),
            color=BenchmarkPlot.colors[pseudo_idx],
        )

        total_time_axis.bar(
            x_axis_adjusted,
            total_time_y_values,
            width=width,
            label=(
                (mapping or dict()).get(
                    category,
                    (
                        f"{parsed_category} Capture"
                        if parsed_category != "SmokedDuck"
                        else "SmokedDuck"
                    ),
                )
            ),
            color=BenchmarkPlot.colors[pseudo_idx],
            yerr=total_time_stdev,
        )

        stdev_base_axis.bar(
            x_axis_adjusted,
            _get_key_in_dict("base_profile_stdev"),
            width=width,
            label=parsed_category,
            color=BenchmarkPlot.colors[pseudo_idx],
        )
        stdev_capture_axis.bar(
            x_axis_adjusted,
            _get_key_in_dict("capture_profile_stdev"),
            width=width,
            label=parsed_category,
            color=BenchmarkPlot.colors[pseudo_idx],
        )
        infer_time_axis.bar(
            x_axis_adjusted,
            y_infer_time_values,
            width=width,
            label=(
                (mapping or dict()).get(
                    category,
                    (
                        f"{parsed_category} Backtrace"
                        if parsed_category != "SmokedDuck"
                        else "SmokedDuck"
                    ),
                )
            ),
            yerr=y_infer_time_values_error,
        )
        infer_time_with_index_axis.bar(
            x_axis_adjusted,
            (
                y_infer_time_values
                if parsed_category != "SmokedDuck"
                else y_infer_time_values_with_index
            ),
            width=width,
            label=(
                (mapping or dict()).get(
                    category,
                    (
                        f"{parsed_category} Backtrace"
                        if parsed_category != "SmokedDuck"
                        else "SmokedDuck w/ Idx Bld"
                    ),
                )
            ),
            yerr=y_infer_time_values_error,
        )
        # if parsed_category == "SmokedDuck":
        #     pseudo_idx += 1
        #     x_axis_adjusted = x_axis + width * pseudo_idx
        #     infer_time_axis.bar(
        #         x_axis_adjusted,
        #         y_infer_time_values_with_index,
        #         width=width,
        #         label=(
        #             (mapping or dict()).get(
        #                 category,
        #                 (
        #                     f"{parsed_category} Backtrace"
        #                     if parsed_category != "SmokedDuck"
        #                     else "SmokedDuck with index build time"
        #                 ),
        #             )
        #         ),
        #         yerr=y_infer_time_values_error,
        #     )
        if category in LOG_SIZE_CATEGORY:
            for (
                log_size_single_axis,
                log_size_single_value,
                log_size_single_stdev,
            ) in zip(log_size_axis, log_size_values, log_size_stdev, strict=True):
                log_size_single_axis.bar(
                    x_axis_log_adjusted,
                    log_size_single_value,
                    width=log_size_width,
                    label=parsed_category,
                    color=BenchmarkPlot.colors[pseudo_idx],
                    yerr=log_size_single_stdev,
                )
            log_size_incr += 1
        pseudo_idx += 1

    fig_axis.set_ylabel("Rel. Ovh. (%)")
    total_time_axis.set_ylabel("Total Time (s)")
    # fig_axis.set_yscale("log")
    if sf == "1":
        handles, labels = fig_axis.get_legend_handles_labels()
        if extra_labels:
            handles.extend(extra_labels.values())
            labels.extend(extra_labels.keys())
        fig_axis.legend(handles=handles, labels=labels, loc="upper right")
    else:
        fig_axis.legend()
        # fig_axis.legend(loc="right", bbox_to_anchor=(0.95, 0.3))

    infer_time_axis.set_ylabel("Backtrace (s)")
    infer_time_axis.set_yscale("log")
    infer_time_axis.set_xlabel("Query")

    infer_time_with_index_axis.set_ylabel("Backtrace w/ Idx Bld (s)")
    infer_time_with_index_axis.set_yscale("log")
    infer_time_with_index_axis.set_xlabel("Query")
    infer_time_with_index_axis.legend()

    total_time_axis.set_xlabel("Query")
    total_time_axis.set_yscale("log")
    total_time_axis.legend()
    total_time_fig.suptitle(f"End-to-end Time for DuckDB (SF={sf})")
    # infer_time_axis.legend()

    fig_axis.set_xticks(x_axis + width * (len(categories) / 3), x_axis_values)
    total_time_axis.set_xticks(x_axis + width * (len(categories) / 3), x_axis_values)
    # fig_axis.set_yticks([0, 10, 20, 30, 40])
    fig.suptitle(f"DuckDB Relative Overhead & Backtrace Time - SF {sf}")

    infer_time_axis.set_xticks(x_axis + width * (len(categories) / 3), x_axis_values)
    infer_time_with_index_axis.set_xticks(
        x_axis + width * (len(categories) / 3), x_axis_values
    )
    for log_size_axe in log_size_axis:
        log_size_axe.set_xticks(
            x_axis + log_size_width * (len(LOG_SIZE_CATEGORY) / 2), x_axis_values
        )

    log_size_axis[0].set_title("Page Requested (Bytes)")
    log_size_axis[1].set_title("Page Used (Bytes)")
    log_size_axis[2].set_title("Bytes Used (Bytes)")

    log_size_axis[0].set_yscale("log")
    log_size_axis[1].set_yscale("log")
    log_size_axis[2].set_yscale("log")
    log_size_axis[2].legend()
    log_size_fig.suptitle(f"Log Size for SF={sf}")

    stdev_capture_axis.set_xticks(x_axis + width * (len(categories) / 2), x_axis_values)
    stdev_capture_axis.legend()
    stdev_capture_axis.set_ylabel("Standard Deviation / Mean")
    stdev_base_axis.set_ylabel("Standard Deviation / Mean")

    stdev_fig.suptitle(f"Standard Deviation / Mean for SF={sf}")
    return fig, log_size_fig, stdev_fig, total_time_fig


def combine_smokedduck_results(all_results: list[dict], parsed: list):
    sd_results = sorted(
        [
            (result, result_parsed)
            for result, result_parsed in zip(all_results, parsed)
            if result_parsed.sd_mode == "old"
        ],
        key=lambda res_parsed: bool(res_parsed[1].sample_inference is not None),
    )
    if len(sd_results) == 1:
        return zip(all_results, parsed)
    assert len(sd_results) == 2
    print("Merging results for SD!")
    all_capture_sd_result = sd_results[0][0]
    sample_inference_sd_result = sd_results[1][0]
    for query, query_result in all_capture_sd_result["results"].items():

        def _copy_key(key_to_copy: str):
            old_position = sample_inference_sd_result["results"][query]["result"]["sd"]
            if key_to_copy in old_position:
                old_value = old_position[key_to_copy]
                assert old_value is None, f"Overwriting: {key_to_copy}"
            old_position[key_to_copy] = query_result["result"]["sd"][key_to_copy]

        _copy_key("capture_profile")
        _copy_key("capture_time")
        _copy_key("capture_stats")
    non_sd_results = [
        res for res in all_results if res not in [sd_res[0] for sd_res in sd_results]
    ]
    non_sd_parsed = [
        parsed_item
        for parsed_item in parsed
        if parsed_item not in [sd_res[1] for sd_res in sd_results]
    ]
    assert len(non_sd_results) == len(all_results) - 2
    filtered_results = [*non_sd_results, sample_inference_sd_result]
    filtered_parsed_items = [*non_sd_parsed, sd_results[1][1]]
    return zip(filtered_results, filtered_parsed_items, strict=True)


def cell_to_num(cell):
    if cell is None:
        return ""
    return str(cell)


def dump_sample_row(rows: list[TpchSampleRow], directory: Path, db_path: Path, table):
    keys = list(rows[0].keys())
    types = [PY_DUCKDB_MAPPING[getattr(rows[0].types(), key)] for key in keys]
    table_columns = [
        f'"{col_name}" {col_type}' for col_name, col_type in zip(keys, types)
    ]
    table_columns_clause = ",".join(table_columns)
    connection = duckdb.connect(db_path)
    cursor = connection.cursor()
    create_table_clause = f"create or replace table {table} ({table_columns_clause})"
    cursor.execute(create_table_clause)
    cursor.close()
    collected_normalized = []

    def _dump_normalized(_collected: list[dict]):
        data_by_key = {key: [] for key in keys}
        for norm_row in _collected:
            for key in data_by_key:
                data_by_key[key].append(norm_row[key])
        df = pd.DataFrame(data_by_key)
        connection.execute(f"insert into {table} SELECT * FROM df")
        _collected.clear()

    for row in rows:
        normalized = row.normalize(preserve_null=True)
        collected_normalized.extend(normalized)
        if len(collected_normalized) == 1000:
            _dump_normalized(collected_normalized)

    if collected_normalized:
        _dump_normalized(collected_normalized)
    connection.close()


def dump_incremental(rows: list[NormalizedRow], directory: Path, db_path: Path, table):
    if isinstance(rows[0], TpchSampleRow):
        dump_sample_row(rows, directory, db_path, table)
        del rows
        return

    connection = duckdb.connect(db_path)
    cursor = connection.cursor()
    all_normalized = []
    for row in rows:
        all_normalized.extend(row.normalize())
    os.makedirs(directory, exist_ok=True)
    path = just_write(directory / "normalized.json", json.dumps(all_normalized))
    cursor.execute(
        f"create or replace table dumped as (select * from read_json_auto('{path}'))"
    )
    cursor.close()
    connection.close()
    del rows


def main():
    plot_context = BenchmarkPlot("analyze_results")
    plot_context.parser.add_argument("-sf", required=True)
    plot_context.parser.add_argument(
        "--use_cache",
        required=False,
        default=False,
        action=argparse.BooleanOptionalAction,
    )
    plot_context.parser.add_argument("--throwaway", required=False, default=5, type=int)
    plot_context.parser.add_argument(
        "--dry_run",
        required=False,
        default=False,
        action=argparse.BooleanOptionalAction,
    )
    parsed = plot_context.parser.parse_args()

    out_dir = Path(parsed.out_dir) / parsed.sf
    os.makedirs(out_dir, exist_ok=True)
    db_path = out_dir / "test.db"

    if not parsed.use_cache:
        all_results = list(plot_context.read_files("result.json", callback=True))
        rows: list[TpchRow] = []
        all_sample_data: list[TpchSampleRow] = []

        bench_parser = make_duckdb_parse()
        add_query_options(bench_parser)

        assert len(all_results) != 0

        # parsed_contents = []
        # for file, contents in all_results:
        #     # sample_inference_result = query_data[]
        #     # for query_name, query_data
        #     call_options = contents["call_options"]
        #     bench_parsed, _ = bench_parser.parse_known_args(call_options)
        #     parsed_contents.append(bench_parsed)
        #     print("read file: ", file)

        for file_name, contents_callback in all_results:
            contents = contents_callback.get_content()
            call_options = contents["call_options"]
            bench_parsed, _ = bench_parser.parse_known_args(call_options)
            print("On file: ", file_name, call_options, bench_parsed, sep="\n")
            if bench_parsed.sd_mode is not None:
                category = "SmokedDuck"
                getter = get_sd_base
            else:
                category = DuckDBDriverOptions.get_suffix(bench_parsed)
                getter = None
            for query_name, query_data in contents["results"].items():
                row_data, sample_data = analyze_result_item(
                    query_data, parsed.throwaway, getter
                )
                rows.append(
                    TpchRow(query_num=query_name, category=category, **row_data)
                )
                for _sample in sample_data:
                    all_sample_data.append(
                        TpchSampleRow(
                            query_num=query_name, category=category, **_sample
                        )
                    )
            contents_callback.delete()
            del contents

        dump_incremental(rows, out_dir / "normalized", db_path, "dumped")
        dump_incremental(
            all_sample_data,
            out_dir / "normalized_sample",
            db_path,
            "dumped_sample",
        )

    if parsed.dry_run:
        return

    conn = duckdb.connect(db_path)
    cursor = conn.cursor()
    sql_query = just_read("./query.sql")
    cursor.execute(sql_query)
    fetched_result = cursor.fetchall()
    for fetched in fetched_result:
        print(fetched[0])

    # fig, log_size_fig, stdev_fig =
    # fig.savefig(out_dir / "capture_backtrace.pdf", bbox_inches="tight")
    # log_size_fig.savefig(out_dir / "log_size.pdf", bbox_inches="tight")
    # stdev_fig.savefig(out_dir / "stdev.pdf", bbox_inches="tight")

    fig, log_size_fig, stdev_fig, total_time_fig = plot_result(
        fetched_result,
        parsed.sf,
        categories=INTERESTING_CATEGORIES,
        mapping=INTERESTING_CATEGORIES_MAP,
        width=0.15,
        figsize=(14, 5),
    )
    fig.savefig(out_dir / "interesting_capture_backtrace.pdf", bbox_inches="tight")
    log_size_fig.savefig(out_dir / "interesting_log_size.pdf", bbox_inches="tight")
    stdev_fig.savefig(out_dir / "interesting_stdev.pdf", bbox_inches="tight")
    total_time_fig.savefig(out_dir / "interesting_total_time.pdf", bbox_inches="tight")
    command = " ".join(sys.argv)
    just_write(out_dir / "command.txt", command)

    plot_box_plot(cursor, sf=parsed.sf, out_dir=out_dir)

    plot_repeat(
        cursor,
        parsed.sf,
        out_dir,
        fetched_result,
        categories=[
            "SmokedDuck",
            # "optimized-y__threads-1__compact-y__merge_chunks-y__table_stats-y",
            "optimized-n__threads-1__filter_pushdown-y__join_filter_rewrite-y__partition_in_agg-y",
        ],
        label=None,
    )


if __name__ == "__main__":
    main()
