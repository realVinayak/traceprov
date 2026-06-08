import argparse
import json
import os
from pathlib import Path
import sys
from typing import Tuple

from matplotlib import pyplot as plt
import matplotlib as mpl

import numpy as np

from traceprovpy.tools.file_utils import (
    json_read_file,
    just_read,
    just_write,
    null_safe,
)
from traceprovpy.tools.normalized_row import Extendable, Normalizable

import duckdb
from traceprovpy.tools.plot_utils import (
    BenchmarkPlot,
    get_unique_handles_labels,
    setup_tpch_analyzer_parser,
    slice_filter,
)


class NormalizedPgTPCHRow(Normalizable):
    category: str
    query_num: str
    phase_1: Extendable
    phase_2: Extendable
    log_size: Extendable

    def keys(self):
        return {"category", "query_num", "phase_1", "phase_2", "log_size"}


def handle_traceprov_infer(infer_extras: dict):
    core_results = infer_extras["core_results"]
    total_time = sum(
        [
            core_result["raw_results"][-1]["result"]["explain_time"]
            for core_result in core_results
        ]
    )
    row_counts = sum([core_result["row_count"] for core_result in core_results])
    return dict(time=total_time, row_count=row_counts)


def handle_traceprov_log_size(log_size_extras: dict):
    return json.loads(log_size_extras["captured"][0][0])


def handle_q15_traceprov(query_num: str, query_data: dict) -> dict:
    traceprov_data = query_data["traceprov_15_skippable"]
    phase_1 = [
        dict(explain_time=item["traceprov"][0]["explain_time"])
        for item in traceprov_data["extras"]
    ]
    phase_2 = [
        handle_traceprov_infer(item["traceprov_infer"][0])
        for item in traceprov_data["extras"]
    ]
    log_size = [
        handle_traceprov_log_size(item["traceprov_get_total_layer_size"][0])
        for item in traceprov_data["extras"]
    ]
    args = dict(
        query_num=query_num,
        phase_1=Extendable(phase_1),
        phase_2=Extendable(phase_2),
        log_size=Extendable(log_size),
    )
    return args


def handle_traceprov(query_num: str, query_data: dict) -> dict:
    if query_num == "15":
        return handle_q15_traceprov(query_num, query_data)
    traceprov_data = query_data["traceprov"]
    phase_1 = [
        dict(explain_time=item["explain_time"]) for item in traceprov_data["base"]
    ]
    phase_2 = [
        handle_traceprov_infer(extra["traceprov_infer"][0])
        for extra in traceprov_data["extras"]
    ]
    log_size = [
        handle_traceprov_log_size(extra["traceprov_get_total_layer_size"][0])
        for extra in traceprov_data["extras"]
    ]
    args = dict(
        query_num=query_num,
        phase_1=Extendable(phase_1),
        phase_2=Extendable(phase_2),
        log_size=Extendable(log_size),
    )
    return args


def handle_q15_base(query_num: str, query_data: dict) -> dict:
    base_data = query_data["base_15_skippable"]
    phase_1 = [
        dict(
            explain_time=item["base"][0]["explain_time"],
            row_count=item["base"][0]["complete_plan"],
        )
        for item in base_data["extras"]
    ]
    return dict(
        query_num=query_num, phase_1=Extendable(phase_1), phase_2=None, log_size=None
    )


def parse_row_count(complete_plan):
    plan = json.loads(complete_plan)
    return plan["Plan"]["Actual Rows"]


def handle_base(query_num: str, query_data: dict, handle_special=True) -> dict:
    if query_num == "15" and handle_special:
        return handle_q15_base(query_num, query_data)
    base_data = query_data["base"]
    contains_timeout = (
        any("timeout" in item for item in base_data["base"])
        if "base" in base_data
        else "timeout" in base_data
    )
    if len(base_data) == 0:
        raise Exception("expected base to be set")
    if contains_timeout:
        phase_1 = [dict(explain_time=-1)]
    else:
        phase_1 = [
            dict(
                explain_time=item["explain_time"],
                row_count=parse_row_count(item["complete_plan"]),
            )
            for item in base_data["base"]
        ]
    return dict(
        query_num=query_num,
        phase_1=Extendable(phase_1) if phase_1 is not None else None,
        phase_2=None,
        log_size=None,
    )


def make_log_size(size: int):
    return dict(bytes_used_size=int(size))


def handle_q15_muller(query_num, query_data):
    muller_data = query_data["traceprov_15_skippable"]
    phase_1 = [
        dict(explain_time=item["phase_1_capture"][0]["explain_time"])
        for item in muller_data["extras"]
    ]
    phase_2 = [
        dict(time=item["phase_2_capture"][0]["explain_time"])
        for item in muller_data["extras"]
    ]
    log_size = [
        make_log_size(item["muller_get_log_size"][0]["captured"][0][0])
        for item in muller_data["extras"]
    ]
    args = dict(
        query_num=query_num,
        phase_1=Extendable(phase_1),
        phase_2=Extendable(phase_2),
        log_size=Extendable(log_size),
    )
    return args


def handle_muller(query_num: str, query_data: dict) -> dict:
    if query_num == "15":
        return handle_q15_muller(query_num, query_data)
    muller_data = query_data["phase_1_2_combined"]
    base = muller_data["base"]
    mat = muller_data["materialize"]
    base_contains_timeout = (
        any("timeout" in item for item in base) or len(base) == 0 or len(mat) == 0
    )
    mat_contains_timeout = any("timeout" in item for item in mat)
    # if len(base) == 0 or len(mat) == 0:
    #     raise Exception("Expected to be set!")
    if base_contains_timeout or mat_contains_timeout:
        phase_1 = [dict(explain_time=-1)]
    else:
        phase_1 = [dict(explain_time=item["explain_time"]) for item in base]
    if mat_contains_timeout:
        phase_2 = [dict(explain_time=-1) for _ in range(len(phase_1))]
    else:
        phase_2 = [dict(time=item["explain_time"]) for item in mat]
    log_size = [
        make_log_size(item["muller_get_log_size"][0]["captured"][0][0])
        for item in muller_data["extras"]
    ]
    if len(log_size) < len(phase_1):
        log_size = None
    args = dict(
        query_num=query_num,
        phase_1=Extendable(phase_1),
        phase_2=Extendable(phase_2),
        log_size=Extendable(log_size) if log_size is not None else None,
    )
    return args


def get_first(result):
    assert len(result) == 1
    return list(result.values())[0]


class ResultAnalyzer:

    def __init__(self):
        self.added_base = False

    def gprom(self, key: str, path: Path) -> list[NormalizedPgTPCHRow]:
        print("Handling GProM!", key)
        result = json_read_file(path, True)
        query_results = get_first(result["result"])
        rows: list[NormalizedPgTPCHRow] = []
        for query_num, query_data in query_results.items():
            for category, category_data in query_data.items():
                base_row_args = handle_base(
                    query_num, dict(base=category_data), handle_special=False
                )
                category_key = f"{key}_{category}"
                base_args = {**base_row_args, "category": category_key}
                rows.append(NormalizedPgTPCHRow(**base_args))
        return rows

    def muller(self, key: str, path: Path) -> list[NormalizedPgTPCHRow]:
        print("Handling Muller!", key)
        result = json_read_file(path, True)
        query_results = result["result"]["muller_params_default"]
        rows: list[NormalizedPgTPCHRow] = []
        for query_num, query_data in query_results.items():
            row_args = handle_muller(query_num, query_data)
            args = {**row_args, "category": key}
            rows.append(NormalizedPgTPCHRow(**args))
        return rows

    def traceprov(
        self,
        key: str,
        path: Path,
    ) -> list[NormalizedPgTPCHRow]:
        print("Handling TraceProv!", key)
        result = json_read_file(path, True)
        query_results = get_first(result["result"])
        base_rows: list[NormalizedPgTPCHRow] = []
        traceprov_rows: list[NormalizedPgTPCHRow] = []
        for query_num, query_data in query_results.items():
            tp_row_args = handle_traceprov(query_num, query_data)
            tp_args = {**tp_row_args, "category": key}
            traceprov_rows.append(NormalizedPgTPCHRow(**tp_args))
            base_row_args = handle_base(query_num, query_data)
            base_args = {**base_row_args, "category": "base"}
            base_rows.append(NormalizedPgTPCHRow(**base_args))
        all_rows = [*traceprov_rows]
        if not self.added_base:
            all_rows = [*base_rows, *all_rows]
        self.added_base = True
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
):
    x_axis_values = list(map(str, range(1, 23)))
    x_axis = np.arange(len(x_axis_values))
    slowdown_fig, slowdown_axis = plt.subplots(1, 1, figsize=(14, 3))
    total_time_fig, total_time_axis = plt.subplots(1, 1, figsize=(14, 3))
    data = [item for item in data if item[0] in categories]
    result_sorted = sorted(data, key=lambda x: categories.index(x[0]))
    q11_idx = x_axis_values.index("11")
    pending_bars = []

    total_time_max = 0
    slowdown_max = 0
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

        y_values = _get_key_in_dict("phase_1_explain_time_mean")
        y_values_end_to_end = _get_key_in_dict("phase_total_time_median")
        y_values_end_to_end_error = _get_key_in_dict("phase_total_time_stdev")
        y_timeout_values = [
            y_idx for y_idx, y in enumerate(y_values) if -1.5 < y and y < -0.5
        ]
        phase_all_slowdown = _get_key_in_dict("phase_all_slowdown")
        y_values_end_to_end_filtered = slice_filter(
            y_values_end_to_end, y_timeout_values
        )
        y_values_end_to_end_error = slice_filter(
            y_values_end_to_end_error, y_timeout_values
        )
        phase_all_slowdown = slice_filter(phase_all_slowdown, y_timeout_values)
        slowdown_max = max(slowdown_max, *phase_all_slowdown)
        total_time_max = max(total_time_max, *y_values_end_to_end_filtered)
        if category_idx > 0:
            x_axis_adjusted_slowdown = x_axis * (
                len(categories) * width + group_gap
            ) + width * (category_idx - 1)
            slowdown_axis.bar(
                x_axis_adjusted_slowdown,
                phase_all_slowdown,
                width=width,
                label=(category_label_mapping or dict()).get(category, category),
                color=BenchmarkPlot.colors[category_idx],
            )
        total_time_axis.bar(
            x_axis_adjusted,
            y_values_end_to_end_filtered,
            width=width,
            label=(category_label_mapping or dict()).get(category, category),
            color=BenchmarkPlot.colors[category_idx],
            yerr=y_values_end_to_end_error,
        )
        if y_timeout_values:
            pending_bars.append(
                dict(
                    x=[x_axis_adjusted[idx] for idx in y_timeout_values],
                    height=10**5,
                    width=width,
                    label="Timeout (> 10 min)",
                    color="lightgray",
                    hatch="///",
                    edgecolor="red",
                )
            )
    for bar in pending_bars:
        slowdown_axis.bar(**{**bar, "height": 2 * slowdown_max})
        total_time_axis.bar(**{**bar, "height": 2 * total_time_max})

    shared_axis = [slowdown_axis, total_time_axis]
    for shared_axes in shared_axis:
        labels_all, handles_all = get_unique_handles_labels(shared_axes)
        print("Labels ", labels_all)
        shared_axes.legend(
            handles=handles_all,
            labels=labels_all,
            loc="upper right",
            bbox_to_anchor=(1.1, 1.2),
            prop=dict(size=8),
        )
        shared_axes.set_xticks(
            x_axis * (len(categories) * width + group_gap)
            + width * (len(categories) / 3),
            x_axis_values,
        )
        shared_axes.set_xlabel("Query")

    slowdown_axis.set_ylim(bottom=0.5, top=100)
    slowdown_axis.annotate(
        "~3000x",
        xy=(q11_idx * (len(categories) * width + group_gap + 0.3), 50),
        ha="center",
        fontsize=14,
        color="red",
        fontweight="bold",
    )

    slowdown_axis.axhline(y=1, color="r", linestyle="--")
    slowdown_axis.axhline(y=2, color="r", linestyle="--")
    slowdown_axis.set_ylabel("Slowdown")

    slowdown_axis.set_yscale("log")
    slowdown_fig.suptitle(f"PostgreSQL Slowdown for SF={sf}")
    slowdown_fig.savefig(out_dir / f"{label}_slowdown.pdf", bbox_inches="tight")

    total_time_axis.set_yscale("log")
    total_time_axis.set_ylabel("Total end-to-end time (s)")
    total_time_fig.suptitle(f"End-to-end Time for Postgres (SF={sf})")
    total_time_fig.savefig(out_dir / f"{label}_end_to_end.pdf", bbox_inches="tight")


def gen_plots(database_file: Path, out_dir: Path, sf):
    con = duckdb.connect(database_file)
    cursor = con.cursor()
    query = """
    SELECT category, list(normalized_slowdown) AS rows
    FROM normalized_slowdown
    GROUP BY category;
    """
    cursor.execute(query)
    results = cursor.fetchall()
    cursor.close()
    con.close()
    result_mapped = [
        (key, {query_data["query_num"]: query_data for query_data in data})
        for (key, data) in results
    ]
    all_categories = [
        "base",
        "gprom_join",
        "gprom_window",
        "gprom_join_heuristics",
        "gprom_window_heuristics",
        "muller",
        "traceprov_no_stats",
        "traceprov_stats",
    ]
    interesting_categories = [
        "base",
        # "gprom_join_heuristics",
        "gprom_window_heuristics",
        "muller",
        "traceprov_stats",
    ]
    plot_data(result_mapped, all_categories, "all", sf, out_dir, 0.1, 0.0)
    mapping = {
        "base": "Baseline",
        # "gprom_join_heuristics": "GProM Join. Heu.",
        "gprom_window_heuristics": "GProM Win. Heu.",
        "muller": "Muller",
        "traceprov_stats": "TraceProv",
    }
    plot_data(
        result_mapped,
        interesting_categories,
        "postgres_interesting",
        sf,
        out_dir,
        0.7,
        0.8,
        mapping,
    )


def main():
    parsed, out_dir = setup_tpch_analyzer_parser(mpl)
    config = json_read_file(parsed.config, True)
    assert config is not None
    in_dir = Path(parsed.dir)
    normalized_rows: list[NormalizedPgTPCHRow] = []
    analyze = ResultAnalyzer()
    for key, item in config.items():
        result_file_path: Path = in_dir / item["file"] / "main_result.json"
        assert result_file_path.exists(), f"Expected {result_file_path} to exist!"
        _rows = getattr(analyze, item["handler"])(key, result_file_path)
        normalized_rows.extend(_rows)

    flatted = [row for group in normalized_rows for row in group.normalize()]
    path = just_write(out_dir / "flat.json", json.dumps(flatted))
    if isinstance(path, Path):
        path = path.as_posix()
    db_file = out_dir / "analyze.db"
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
    call_options = " ".join(sys.argv)
    just_write(out_dir / "call_options.txt", call_options)
    gen_plots(db_file, out_dir, parsed.sf)


if __name__ == "__main__":
    main()
