import argparse
from functools import reduce
import json
import math
import os
from pathlib import Path
import statistics
from typing import Callable, Dict

from matplotlib import pyplot as plt
import numpy as np

from traceprovpy.tools.duckdb_inference import DuckDBDriverOptions
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse
from traceprovpy.tools.file_utils import just_read, just_write
from traceprovpy.tools.normalized_row import (
    Extendable,
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


class TpchRow(NormalizedSampleInferRow):
    query_num: str
    # dict because there are multiple types of this.
    log_sizes: Extendable
    mean_stdev: float

    def keys(self):
        return super().keys() | {"query_num", "log_sizes", "mean_stdev"}


# basically flattens keys into individual values.
def _reduce_keys(prev: dict, curr: dict):
    return {
        **prev,
        **{key: [*prev.get(key, []), value] for (key, value) in curr.items()},
    }


def remove_throwaway(in_values: list):
    assert len(in_values) == 15
    return in_values[5:]


def aggregate_result(results: list[dict]):
    flattened = reduce(_reduce_keys, results, dict())
    median_values = {
        f"{key}_median": statistics.median(remove_throwaway(values))
        for (key, values) in flattened.items()
    }
    stdev_ratio_values = {
        f"{key}_std_ratio": statistics.stdev(remove_throwaway(values))
        / statistics.mean(remove_throwaway(values))
        for (key, values) in flattened.items()
    }
    std_values = {
        f"{key}_std": statistics.stdev(remove_throwaway(values))
        for (key, values) in flattened.items()
    }
    agg = {**median_values, **stdev_ratio_values, **std_values}
    return agg


def aggregate_over_samples(results: list[dict]):
    return dict(
        average_time=statistics.mean(result["latency_median"] for result in results),
        max_stdev_ratio=max(result["latency_std_ratio"] for result in results),
        max_stdev=max(result["latency_std"] for result in results),
        mean_stdev=math.sqrt(
            sum(result["latency_std"] ** 2 for result in results) / len(results)
        ),
    )


def get_sd_base(result_item: dict):
    return result_item["sd"]


def analyze_result_item(result_item: dict, base_getter: Callable[[Dict], Dict] = None):
    base_result_item = result_item["result"]
    if base_getter:
        base_result_item = base_getter(base_result_item)
    base_result = base_result_item["base_profile"]
    sample_infer_result = result_item["sample_inference_result"]
    capture_time_items = None
    capture_profile_items = None
    log_sizes = None
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

        for profile_entry_idx, profile_entry in enumerate(
            sample_infer_result["profile"]
        ):
            if profile_entry_idx in capture_indexes:
                continue
            first_key, iter_id = sql_spec_map[profile_entry_idx]
            assert len(first_key) in (2, 3)
            if len(first_key) == 2:
                part_key = first_key
            else:
                part_key = first_key[1:]

            # print("using keys: ", part_key, first_key)
            part_key = tuple(part_key)
            if part_key not in sample_result:
                sample_result[part_key] = [list() for _ in range(len(capture_indexes))]
            sample_result[part_key][iter_id].append(tap_profile_result(profile_entry))

        # need to reduce the result up a bit
        sample_result_reduced = aggregate_over_samples(
            [
                aggregate_result(
                    [
                        reduce(sum_simple_result, iter_result)
                        for iter_result in sample_results
                    ]
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
            average_time=None, max_stdev_ratio=None, max_stdev=None, mean_stdev=None
        )
        capture_time_items = base_result_item["capture_time"]
        capture_profile_items = base_result_item["capture_profile"]
        capture_indexes = None

    if base_getter is not None:
        # need to map the log sizes too.
        log_sizes = [
            dict(
                page_requested_size=capture_stat["size_mb"] * (1024 * 1024),
                page_used_size=capture_stat["size_mb"] * (1024 * 1024),
                bytes_used_size=capture_stat["size_mb"] * (1024 * 1024),
            )
            for capture_stat in base_result_item["capture_stats"]
        ]
    else:
        log_sizes = [
            capture_time["option"]["misc_key_value_total_log_size"][0]
            for capture_time in capture_time_items
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
    )

    return {**base_and_capture, **sample_result_reduced}


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

LOG_SIZE_CATEGORY = [
    "SmokedDuck",
    "optimized-y__threads-1",
    "optimized-y__threads-1__compact-y",
    "optimized-y__threads-1__partition_in_agg-y",
    "optimized-y__threads-1__compact-y__partition_in_agg-y",
]


def plot_result(fetched_result: list[tuple[str, list]], sf: str):
    x_axis_values = list(map(str, range(1, 23)))
    x_axis = np.arange(len(x_axis_values))
    width = 0.1
    log_size_width = 0.15
    fig, (fig_axis, infer_time_axis) = plt.subplots(2, 1, figsize=(20, 6))
    log_size_fig, log_size_axis = plt.subplots(1, 3, figsize=(25, 6), sharey=True)
    stdev_fig, (stdev_base_axis, stdev_capture_axis) = plt.subplots(
        2, 1, figsize=(20, 6), sharex=True
    )
    result_sorted = sorted(fetched_result, key=lambda x: CATEGORIES.index(x[0]))
    log_size_incr = 0
    fig_axis.axhline(y=10, color="r", linestyle="--")
    fig_axis.axhline(y=20, color="r", linestyle="--")
    for category_idx, (category, category_data) in enumerate(result_sorted):

        remapped_data = {item["query_num"]: item for item in category_data}
        parsed_category = parse_category(category)
        x_axis_adjusted = x_axis + width * category_idx
        x_axis_log_adjusted = x_axis + log_size_width * log_size_incr

        def _get_key_in_dict(in_key: str):
            return null_safe(
                [
                    remapped_data[q][in_key] if q in remapped_data else 0
                    for q in x_axis_values
                ]
            )

        y_values = _get_key_in_dict("relative_overhead")
        y_infer_time_values = _get_key_in_dict("average_time")
        # y_infer_time_values_error = _get_key_in_dict("mean_stdev")
        y_infer_time_values_error = _get_key_in_dict("max_stdev")
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
                f"{parsed_category} Capture"
                if parsed_category != "SmokedDuck"
                else "SmokedDuck (Phase I)"
            ),
            color=BenchmarkPlot.colors[category_idx],
        )

        stdev_base_axis.bar(
            x_axis_adjusted,
            _get_key_in_dict("base_profile_stdev"),
            width=width,
            label=parsed_category,
            color=BenchmarkPlot.colors[category_idx],
        )
        stdev_capture_axis.bar(
            x_axis_adjusted,
            _get_key_in_dict("capture_profile_stdev"),
            width=width,
            label=parsed_category,
            color=BenchmarkPlot.colors[category_idx],
        )
        infer_time_axis.bar(
            x_axis_adjusted,
            y_infer_time_values,
            width=width,
            label=(
                f"{parsed_category} Backtrace"
                if parsed_category != "SmokedDuck"
                else "SmokedDuck (Phase II)"
            ),
            yerr=y_infer_time_values_error,
        )
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
                    color=BenchmarkPlot.colors[category_idx],
                    yerr=log_size_single_stdev,
                )
            log_size_incr += 1

    fig_axis.set_ylabel("Relative Overhead (%)")
    # fig_axis.set_yscale("log")
    if sf == "1":
        fig_axis.legend(bbox_to_anchor=(0.95, 0.3))
    else:
        fig_axis.legend(loc=(0.95, 0.3))
        # fig_axis.legend(loc="right", bbox_to_anchor=(0.95, 0.3))

    infer_time_axis.set_ylabel("Backtrace Time (s)")
    infer_time_axis.set_yscale("log")
    infer_time_axis.legend(bbox_to_anchor=(0.95, 1))

    fig_axis.set_xticks(x_axis + width * (len(CATEGORIES) / 2), x_axis_values)
    fig.suptitle(f"Relative Overhead and Backtrace Time for SF={sf}")

    infer_time_axis.set_xticks(x_axis + width * (len(CATEGORIES) / 2), x_axis_values)
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
    log_size_axis[2].legend(bbox_to_anchor=(0.95, 0.5))
    log_size_fig.suptitle(f"Log Size for SF={sf}")

    stdev_capture_axis.set_xticks(x_axis + width * (len(CATEGORIES) / 2), x_axis_values)
    stdev_capture_axis.legend(bbox_to_anchor=(0.95, 0.5))
    stdev_capture_axis.set_ylabel("Standard Deviation / Mean")
    stdev_base_axis.set_ylabel("Standard Deviation / Mean")

    stdev_fig.suptitle(f"Standard Deviation / Mean for SF={sf}")
    return fig, log_size_fig, stdev_fig


def combine_smokedduck_results(all_results: list[dict], parsed: list):
    sd_results = sorted(
        [
            (result, result_parsed)
            for result, result_parsed in zip(all_results, parsed)
            if result_parsed.sd_mode == "old"
        ],
        key=lambda res_parsed: bool(res_parsed[1].sample_inference is not None),
    )
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


def main():
    plot_context = BenchmarkPlot("analyze_results")
    plot_context.parser.add_argument("-sf", required=True)
    plot_context.parser.add_argument(
        "--use_cache",
        required=False,
        default=False,
        action=argparse.BooleanOptionalAction,
    )
    parsed = plot_context.parser.parse_args()

    out_dir = Path(parsed.out_dir) / parsed.sf
    os.makedirs(out_dir, exist_ok=True)

    conn = duckdb.connect(out_dir / "test.db")
    cursor = conn.cursor()

    if not parsed.use_cache:
        all_results = list(plot_context.read_files("result.json"))
        rows: list[TpchRow] = []

        bench_parser = make_duckdb_parse()
        add_query_options(bench_parser)

        assert len(all_results) != 0

        parsed_contents = []
        for file, contents in all_results:
            # sample_inference_result = query_data[]
            # for query_name, query_data
            call_options = contents["call_options"]
            bench_parsed, _ = bench_parser.parse_known_args(call_options)
            parsed_contents.append(bench_parsed)
            print("read file: ", file)

        for contents, bench_parsed in combine_smokedduck_results(
            [con for (_, con) in all_results], parsed_contents
        ):
            if bench_parsed.sd_mode is not None:
                category = "SmokedDuck"
                getter = get_sd_base
            else:
                category = DuckDBDriverOptions.get_suffix(bench_parsed)
                getter = None
            for query_name, query_data in contents["results"].items():
                row_data = analyze_result_item(query_data, getter)
                rows.append(
                    TpchRow(query_num=query_name, category=category, **row_data)
                )

        all_normalized = []
        for row in rows:
            all_normalized.extend(row.normalize())
        path = just_write(out_dir / "normalized.json", json.dumps(all_normalized))
        if isinstance(path, Path):
            path = path.as_posix()

        cursor.execute(
            f"create or replace table dumped as (select * from read_json_auto('{path}'))"
        )

    sql_query = just_read("./query.sql")
    cursor.execute(sql_query)
    fetched_result = cursor.fetchall()
    for fetched in fetched_result:
        print(fetched[0])
    fig, log_size_fig, stdev_fig = plot_result(fetched_result, parsed.sf)
    fig.savefig(out_dir / "capture_backtrace.png")
    log_size_fig.savefig(out_dir / "log_size.png")
    stdev_fig.savefig(out_dir / "stdev.png")


if __name__ == "__main__":
    main()
