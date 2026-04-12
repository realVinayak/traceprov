import argparse
from functools import reduce
import json
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

    def keys(self):
        return super().keys() | {"query_num"}


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
    )


def get_sd_base(result_item: dict):
    return result_item["sd"]


def analyze_result_item(result_item: dict, base_getter: Callable[[Dict], Dict] = None):
    base_result_item = result_item["result"]
    if base_getter:
        base_result_item = base_getter(base_result_item)
    base_result = base_result_item["base_profile"]
    sample_infer_result = result_item["sample_inference_result"]
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
    else:
        sample_result_reduced = dict(
            average_time=None, max_stdev_ratio=None, max_stdev=None
        )
        capture_indexes = None

    base_and_capture = dict(
        base=Extendable(map(tap_simple_result, base_result_item["base_time"])),
        base_profile=Extendable(
            map(tap_profile_result, base_result_item["base_profile"])
        ),
        capture=(
            None
            if capture_indexes is None
            # TODO: Maybe handle the case when extendables are 0?
            else Extendable(
                map(
                    tap_simple_result,
                    [
                        sample_infer_result["result_time"][l_idx]
                        for l_idx in capture_indexes
                    ],
                )
            )
        ),
        capture_profile=(
            None
            if capture_indexes is None
            else Extendable(
                map(
                    tap_profile_result,
                    [
                        sample_infer_result["profile"][l_idx]
                        for l_idx in capture_indexes
                    ],
                )
            )
        ),
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


def plot_result(fetched_result: list[tuple[str, list]]):
    x_axis_values = list(map(str, range(1, 23)))
    x_axis = np.arange(len(x_axis_values))
    width = 0.1
    fig, (fig_axis, infer_time_axis) = plt.subplots(2, 1, figsize=(20, 6))
    result_sorted = sorted(fetched_result, key=lambda x: CATEGORIES.index(x[0]))
    for category_idx, (category, category_data) in enumerate(result_sorted):

        remapped_data = {item["query_num"]: item for item in category_data}
        parsed_category = parse_category(category)
        x_axis_adjusted = x_axis + width * category_idx
        y_values = null_safe(
            [
                remapped_data[q]["relative_overhead"] if q in remapped_data else 0
                for q in x_axis_values
            ]
        )
        y_infer_time_values = null_safe(
            [
                remapped_data[q]["average_time"] if q in remapped_data else 0
                for q in x_axis_values
            ]
        )
        y_infer_time_values_error = null_safe(
            [
                remapped_data[q]["max_stdev"] if q in remapped_data else 0
                for q in x_axis_values
            ]
        )
        print(y_values)
        fig_axis.bar(
            x_axis_adjusted,
            y_values,
            width=width,
            label=(
                f"{parsed_category} Capture"
                if parse_category != "SmokedDuck"
                else "SmokedDuck (Phase I)"
            ),
            color=BenchmarkPlot.colors[category_idx],
        )
        infer_time_axis.bar(
            x_axis_adjusted,
            y_infer_time_values,
            width=width,
            label=(
                f"{parsed_category} Backtrace"
                if parse_category != "SmokedDuck"
                else "SmokedDuck (Phase II)"
            ),
            yerr=y_infer_time_values_error,
        )
    fig_axis.set_xticks(x_axis + (len(fetched_result) / 2) * width, x_axis_values)
    fig_axis.set_ylabel("Relative Overhead (%)")
    fig_axis.set_yscale("log")
    fig_axis.legend(bbox_to_anchor=(0.95, 1.1))

    infer_time_axis.set_xticks(
        x_axis + (len(fetched_result) / 2) * width, x_axis_values
    )
    infer_time_axis.set_ylabel("Backtrace Time (s)")
    infer_time_axis.set_yscale("log")
    infer_time_axis.legend(bbox_to_anchor=(0.95, 1))
    return fig


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

        for file, contents in all_results:
            # sample_inference_result = query_data[]
            # for query_name, query_data
            call_options = contents["call_options"]
            bench_parsed, _ = bench_parser.parse_known_args(call_options)
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

            print("read: ", file)

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
    fig = plot_result(fetched_result)
    fig.savefig(out_dir / "rovh.png")


if __name__ == "__main__":
    main()
