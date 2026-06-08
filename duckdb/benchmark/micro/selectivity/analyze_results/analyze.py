import argparse
import json
import os
from pathlib import Path

from matplotlib import pyplot as plt
import numpy as np
import duckdb

from traceprovpy.tools.duckdb_inference import DuckDBDriverOptions
from traceprovpy.tools.file_utils import just_read, just_write
from traceprovpy.tools.normalized_row import (
    Extendable,
    NormalizedSampleInferRow,
    parse_sample_results,
    tap_profile_result,
    tap_simple_result,
)
from traceprovpy.tools.parser_defs import make_duckdb_selectivity_parser
from traceprovpy.tools.plot_utils import BenchmarkPlot
from traceprovpy.tools.run_duckdb_generic import (
    TRACEPROV_CAPTURE_ENTRY,
    TRACEPROV_CAPTURE_ENTRY_SD,
)


class NormalizedSelectivityInferRow(NormalizedSampleInferRow):
    num_rows: int
    selectivity: int
    category: str
    random: bool
    extra: Extendable

    def keys(self):
        return super().keys() | {
            "extra",
            "num_rows",
            "selectivity",
            "category",
            "random",
        }


def get_row(parsed, all_results):
    is_sd = parsed.sd_mode is not None
    for result in all_results["results"]["result"]:
        dir_name = result["dir"]
        selectivity = result["selectivity"]
        core_result = result["result"]
        sample_result = core_result["sample_inference"]
        if is_sd:
            core_result = core_result["sd"]
        base_result = core_result["base_profile"]
        sql_spec_map = sample_result["sql_spec_map"]
        capture_indexes = [
            l_idx
            for (l_idx, (map_entry, _)) in enumerate(sql_spec_map)
            if tuple(map_entry) in (TRACEPROV_CAPTURE_ENTRY, TRACEPROV_CAPTURE_ENTRY_SD)
        ]
        _, sample_result_reduced = parse_sample_results(
            sample_result, capture_indexes, not is_sd
        )
        try:
            index_building_time = sample_result["stats"][0]["build_time"]
            print("Found index building time: ", index_building_time)
        except KeyError:
            index_building_time = 0
        assert len(capture_indexes) == len(base_result)
        capture_time_items = [
            sample_result["result_time"][l_idx] for l_idx in capture_indexes
        ]
        capture_profile_items = [
            sample_result["profile"][l_idx] for l_idx in capture_indexes
        ]
        if core_result["capture_time"] is None:
            print(parsed)
        extras = [
            dict(index_build_time=index_building_time, part_time=0) for _ in range(15)
        ]
        row_args = dict(
            num_rows=dir_name,
            selectivity=selectivity,
            category="SmokedDuck" if is_sd else DuckDBDriverOptions.get_suffix(parsed),
            base=Extendable(map(tap_simple_result, core_result["base_time"])),
            base_profile=Extendable(
                map(tap_profile_result, core_result["base_profile"])
            ),
            capture=Extendable(map(tap_simple_result, capture_time_items)),
            capture_profile=Extendable(map(tap_profile_result, capture_profile_items)),
            random=parsed.random,
            extra=Extendable(extras),
        )
        row_args = {**row_args, **sample_result_reduced}
        yield row_args


def remap_data(in_data: list):
    return {row["selectivity"]: row for row in in_data}


def plot_result(
    fetched_result: list[tuple[bool, str, list]], out_dir: Path, mapping: dict = None
):
    x_axis_values = [0.01, 0.05, 0.1, 0.5, 1.0, 5, 10, 50, 85.0, 90.0, 95.0]
    x_axis = np.arange(len(x_axis_values))
    index_build_fig, index_build_axis = plt.subplots(1, 1)
    backtrace_time_fig, backtrace_time_axis = plt.subplots(2, 1, figsize=(6, 8))
    x_axis_keys = [int(val * 100) for val in x_axis_values]

    sd_index_build_time = {
        res[0]: remap_data(res[2]) for res in fetched_result if res[1] == "SmokedDuck"
    }

    def _get_key(_result: dict, key):
        return [_result[x_axis_key][key] for x_axis_key in x_axis_keys]

    width = 0.2
    index_build_axis.bar(
        x_axis,
        _get_key(sd_index_build_time[True], "build_time"),
        width=width,
        label="Random Order",
    )
    index_build_axis.bar(
        x_axis + width,
        _get_key(sd_index_build_time[False], "build_time"),
        width=width,
        label="Group-by Sort Order",
    )
    index_build_axis.set_ylabel("Index build time")
    index_build_axis.set_yscale("log")
    index_build_axis.legend()
    index_build_axis.set_xticks(x_axis + width / 2, x_axis_values)
    index_build_axis.set_xlabel("Selectivity")
    index_build_fig.suptitle(f"SmokedDuck Index Build Time (s)")
    index_build_fig.savefig(out_dir / "index_build_time.pdf", bbox_inches="tight")

    for _idx, random_order in enumerate([True, False]):
        system_backtrace_time = {
            mapping[res[1]]: _get_key(remap_data(res[2]), "backtrace_time")
            for res in fetched_result
            if res[1] in mapping and res[0] == random_order
        }
        system_backtrace_time = {
            **system_backtrace_time,
            "SmokedDuck w/ idx bld": _get_key(
                sd_index_build_time[random_order], "total_backtrace_time"
            ),
        }
        system_backtrace_sorted = sorted(
            system_backtrace_time.items(), key=lambda x: x[0]
        )
        for cat_idx, (category_label, category_data) in enumerate(
            system_backtrace_sorted
        ):
            backtrace_time_axis[_idx].bar(
                x_axis + width * cat_idx,
                category_data,
                width=width,
                label=category_label,
            )
        backtrace_time_axis[_idx].set_xticks(
            x_axis + width * len(mapping) / 2, x_axis_values
        )
        backtrace_time_axis[_idx].set_yscale("log")
        backtrace_time_axis[_idx].set_ylabel("Backtrace Time (s)")
        backtrace_time_axis[_idx].legend()
        if _idx == 1:
            backtrace_time_axis[_idx].set_xlabel("Selectivity")
        backtrace_time_axis[_idx].set_title(
            "Random" if random_order else "Group-by order"
        )

    backtrace_time_fig.suptitle(f"Backtrace Time (s)")
    backtrace_time_fig.savefig(out_dir / "backtrace_time.pdf", bbox_inches="tight")


def main():
    parser = BenchmarkPlot("analyze_partition")
    parser.parser.add_argument(
        "--cache", action=argparse.BooleanOptionalAction, default=False
    )
    parsed = parser.parser.parse_args()
    out_dir = Path(parsed.out_dir)
    os.makedirs(out_dir, exist_ok=True)

    conn = duckdb.connect(out_dir / "test.db")
    cursor = conn.cursor()

    if not parsed.cache:
        results = list(parser.read_files("result.json"))
        result_parser = make_duckdb_selectivity_parser()
        with_parsed = [
            (result[1], result_parser.parse_known_args(result[1]["call_options"])[0])
            for result in results
        ]
        norm_rows = [
            NormalizedSelectivityInferRow(**row)
            for result, parsed in with_parsed
            for row in get_row(parsed, result)
        ]
        all_normalized = []
        for row in norm_rows:
            all_normalized.extend(row.normalize())
        path = just_write(out_dir / "normalized.json", json.dumps(all_normalized))
        if isinstance(path, Path):
            path = path.as_posix()

        cursor.execute(
            f"create or replace table dumped as (select * from read_json_auto('{path}'))"
        )

    query = just_read("./query.sql")
    cursor.execute(query)

    pack_result = cursor.fetchall()
    mapping = {
        "optimized-y__threads-1__compact-y__merge_chunks-y__partition_in_agg-y__table_stats-y": "TraceProv",
        "SmokedDuck": "SmokedDuck",
    }
    plot_result(pack_result, out_dir, mapping)


if __name__ == "__main__":
    main()
