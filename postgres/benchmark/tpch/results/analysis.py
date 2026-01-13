from functools import reduce
import glob
import json
from typing import Dict, List, NamedTuple, Tuple
import statistics
import numpy as np
from traceprovpy.tools.plot_utils import *
import matplotlib.pyplot as plt
from pathlib import Path


def is_suitable(in_str: str):
    if not PlotMap.is_key(in_str):
        return True
    _, (thread, _) = PlotMap.strip_thread(in_str)
    return thread in [1, 4, 12]


def handle_legacy(query_result, top_key):
    traceprov_extras = query_result["traceprov"]["extras"]
    extra = traceprov_extras[0]
    if "traceprov_infer_time_1" in extra:
        print("Skipping multiple layers!")
        return {PlotMap.traceprov_infer_legacy: []}
    traceprov_sync = [
        get_captured(extra["traceprov_sync_time"]) / (1_000_000)
        for extra in query_result["traceprov"]["extras"]
    ]
    raw_traceprov_infer = [
        get_captured(extra["traceprov_infer_time_0"]) / (1_000_000)
        for extra in query_result["traceprov"]["extras"]
    ]
    traceprov_infer = list(
        map(sum, zip(traceprov_sync, raw_traceprov_infer, strict=True))
    )
    return {PlotMap.traceprov_infer_legacy: traceprov_infer}


def map_query_result(query_result: dict, tup_key: Tuple[int, bool]):
    if tup_key == (None, None):
        return handle_legacy(query_result, tup_key)
    base_timings = flatten_simple_base(query_result["base"]["base"])
    traceprov_forward = flatten_simple_base(query_result["traceprov"]["base"])
    traceprov_sync = [
        get_captured(extra["traceprov_sync_time"]) / (1_000_000)
        for extra in query_result["traceprov"]["extras"]
    ]
    raw_traceprov_infer = [
        json.loads(get_captured(extra["traceprov_derivation_spec"]))["total_time"]
        / (1_000_000)
        for extra in query_result["traceprov"]["extras"]
    ]
    traceprov_infer = list(
        map(sum, zip(traceprov_sync, raw_traceprov_infer, strict=True))
    )
    traceprov_f_infer = list(
        map(sum, zip(traceprov_forward, traceprov_infer, strict=True))
    )
    duckdb_timings = extract_duckdb_timings(query_result)
    return {
        PlotMap.base: base_timings,
        PlotMap.traceprov: traceprov_forward,
        PlotMap.traceprov_infer: traceprov_infer,
        PlotMap.traceprov_f_infer: traceprov_f_infer,
        PlotMap.annotate_with_key(PlotMap.duckdb_time, tup_key): duckdb_timings,
    }


def assert_is_list(in_value):
    assert isinstance(in_value, list)
    return in_value


def reducer(previous: dict, current: Tuple[int, dict]):
    qnum, qmapped_result = current
    qnum_prev_result: dict = previous.get(qnum, {})
    return {
        **previous,
        qnum: {
            **qnum_prev_result,
            **{
                map_key: [
                    *qnum_prev_result.get(map_key, []),
                    *assert_is_list(map_value),
                ]
                for (map_key, map_value) in qmapped_result.items()
            },
        },
    }


def plot_capture(flipped_result, label, outdir: Path):
    param_result = {
        key: result for (key, result) in flipped_result.items() if key in CAPTURE_ORDER
    }
    fig, (ax, slowdown_plot) = plt.subplots(1, 2, figsize=(15, 6))
    width = 0.2
    previous_x_axis_repr = None
    for cat_idx, (category, category_result) in enumerate(
        sorted(param_result.items(), key=lambda pair: CAPTURE_ORDER.index(pair[0]))
    ):
        x_axis_values = sorted(category_result.keys(), key=lambda x: int(x))
        assert previous_x_axis_repr is None or previous_x_axis_repr == str(
            tuple(x_axis_values)
        )
        previous_x_axis_repr = str(tuple(x_axis_values))
        y_values = [
            guard_no_len(statistics.median)(category_result[x]) for x in x_axis_values
        ]
        rects = ax.bar(
            np.arange(len(x_axis_values)) + (cat_idx * width),
            y_values,
            width,
            label=category,
        )
    # if use_log:
    #     ax.set_yscale("log", base=10)
    ax.legend(loc="upper left", ncols=3)
    ax.set_xticks(np.arange(len(x_axis_values)) + width, x_axis_values)
    ax.set_title("Execution time (s)")

    slowdown_plot.set_xticks(np.arange(len(x_axis_values)) + width, x_axis_values)

    slowdown_results = [
        statistics.median(
            compute_slowdown(
                param_result[PlotMap.traceprov][qnum], param_result[PlotMap.base][qnum]
            )
        )
        for qnum in x_axis_values
    ]
    slowdown_plot.bar(x_axis_values, slowdown_results, width)
    # slowdown_plot.legend(loc="upper right", ncols=3)
    slowdown_plot.axhline(y=10, color="r", linestyle="--", label="10%")
    slowdown_plot.axhline(y=20, color="r", linestyle="--", label="20%")

    slowdown_plot.set_title("Slowdown (%)")

    ax.set_xlabel("Query")
    ax.set_ylabel("Execution time (s)")
    slowdown_plot.set_ylabel("Slowdown  (%)")
    slowdown_plot.set_xlabel("Query")

    fig.suptitle(f"Result: {label}")
    fig.savefig(outdir / f"{label}_capture.png")


def plot_infer_result(flipped_result, label, outdir):
    param_result = [
        (
            (
                (
                    PlotMap.is_key(key),
                    INFER_ORDER.index(key) if key in INFER_ORDER else 0,
                ),
                key,
            ),
            key,
            result,
        )
        for (key, result) in flipped_result.items()
        if is_infer(key) and is_suitable(key)
    ]
    param_result_sorted = sorted(param_result, key=lambda x: x[0])
    fig, ax = plt.subplots(1, 1, figsize=(15, 6))
    width = 0.1
    previous_x_axis_repr = None

    x_axis_values = list(map(str, range(1, 23)))
    for cat_idx, (_, cat_key, category_result) in enumerate(param_result_sorted):
        # x_axis_values = sorted(category_result.keys(), key=lambda x: int(x))
        # print(x_axis_values)
        # assert previous_x_axis_repr is None or previous_x_axis_repr == str(
        #     tuple(x_axis_values)
        # )
        # previous_x_axis_repr = str(tuple(x_axis_values))
        y_values = [
            guard_no_len(statistics.median)(category_result.get(x, []))
            for x in x_axis_values
        ]
        rects = ax.bar(
            np.arange(len(x_axis_values)) + (cat_idx * width),
            y_values,
            width,
            label=PlotMap.get_label(cat_key),
        )
    ax.legend(loc="upper left", ncols=3, prop=dict(size=8))
    ax.set_xticks(np.arange(len(x_axis_values)) + width, x_axis_values)

    ax.set_yscale("log", base=10)
    ax.set_title(f"Inference: {label}")
    ax.set_xlabel("Query")
    ax.set_ylabel("Execution time (s)")
    fig.savefig(outdir / f"{label}_infer.png")


import os


def main():
    bench_plotter = BenchmarkPlot("results_analyzer")
    bench_plotter.parser.add_argument("-o", "--out_dir", required=True)
    bench_plotter.parser.add_argument("-l", "--label", required=True)
    parsed = bench_plotter.parser.parse_args()
    outdir = Path(parsed.out_dir)
    os.system(f"mkdir -p {outdir.as_posix()}")
    all_results = dict()
    assert len(parsed.files) == 1
    result_file = parsed.files[0]
    with open(result_file) as f:
        files = [line.strip() for line in f.readlines() if line.strip() != ""]
    for file in files:
        complete_path = f"{parsed.root}/{file}/main_result.json"
        paths = glob.glob(complete_path)
        for path in paths:
            with open(path) as f:
                raw_result = json.load(f)["result"]
            result = raw_result["params_default"]
            path_obj = Path(path)
            dir_name = path_obj.parts[-2]
            thread_count = None
            print(dir_name)
            thread_count = extract_threads("threads", dir_name)
            if thread_count is None:
                thread_count = extract_threads("thread", dir_name)
            index = extract_idx(dir_name)
            print(dir_name, thread_count, index)
            key = (thread_count, index)
            assert key not in all_results
            all_results[key] = result

    flattened = [
        (qnumber, map_query_result(query_result, key))
        for key, result in all_results.items()
        for (qnumber, query_result) in result.items()
    ]

    reduced_as_dict = reduce(reducer, flattened, dict())

    with open(outdir / "combined_new.json", "w") as f:
        f.write(json.dumps(reduced_as_dict, indent=4))

    limited_results = {
        qnum: {
            cat: (
                cat_result[:10]
                if PlotMap.duckdb_time not in cat_result
                else cat_result[5:]
            )
            for (cat, cat_result) in qresult.items()
        }
        for qnum, qresult in reduced_as_dict.items()
    }

    # with open("combined_stats_limited.json", "w") as f:
    #     f.write(
    #         json.dumps(
    #             dict(
    #                 counts=apply_func(limited_results, len),
    #                 std_dev=apply_func(limited_results, guard_no_len(statistics.stdev)),
    #                 median=apply_func(limited_results, guard_no_len(statistics.median)),
    #                 mean=apply_func(limited_results, guard_no_len(statistics.mean)),
    #             ),
    #             indent=4,
    #         )
    #     )

    flipped = reduce(flipper, limited_results.items(), dict())

    with open(outdir / "combined_flipped.json", "w") as f:
        f.write(
            json.dumps(
                flipped,
                indent=4,
            )
        )

    with open(outdir / "combined_stats_limited.tsv", "w") as f:
        result_mapped = [
            (
                cat,
                q,
                str(len(q_result)),
                str(guard_no_len(statistics.stdev)(q_result)),
                str(guard_no_len(statistics.median)(q_result)),
                str(guard_no_len(statistics.mean)(q_result)),
            )
            for cat, cat_result in flipped.items()
            for (q, q_result) in cat_result.items()
        ]
        headers = ("category", "qnum", "len", "stddev", "median", "mean")
        all_results_flat = [headers, *result_mapped]
        tsv = "\n".join(["\t".join(row) for row in all_results_flat])
        f.write(tsv)

    plot_capture(flipped, parsed.label, outdir)
    plot_infer_result(flipped, parsed.label, outdir)


if __name__ == "__main__":
    main()
