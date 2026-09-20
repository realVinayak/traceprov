from collections import defaultdict
from functools import reduce
import json
import math
from pathlib import Path
import random
from statistics import mean

from matplotlib import pyplot as plt
import numpy as np

from utils import SYSTEM_COLORS, SYSTEM_MARKERS, SystemLabels
from traceprovpy.tools.file_utils import (
    json_read_file,
    just_read,
    just_write,
    run_query,
)
from traceprovpy.tools.plot_utils import (
    BenchmarkPlot,
    add_arrow_label,
    get_unique_handles_labels,
    tp_add_grid_line,
)
import pandas as pd
import duckdb
from plot_sd_comparison import get_percent_break

from matplotlib.transforms import blended_transform_factory

CATEGORY_MAPPING = {
    "traceprov": SystemLabels.traceprov,
    "smokedduck": SystemLabels.smokedduck,
}


def main():
    plot_context = BenchmarkPlot("plot_sd_comparison_zipf", ignore_args=True)
    plot_context.parser.add_argument("--db", required=True)
    parsed = plot_context.parser.parse_args()

    out_dir = plot_context.add_timestamp()
    num_rows = [10_000_000]
    conn = duckdb.connect(parsed.db)
    cursor = conn.cursor()
    # raw_results = fetch_results(cursor, 50_000_000)
    # percent_diff = get_breakeven(raw_results)
    plot_overall_breakeven(json_read_file("./tmp/debug_percent_diff.json"), out_dir)
    plot_total_time(10_000_000, cursor, out_dir)


QUERIES_LIST = [10, 100, 1_000]


def plot_total_time(num_rows, cursor, out_dir):
    results = run_query(
        cursor, set_num_rows(just_read("./overview_offset_query_zipf.sql"), num_rows)
    )
    results = list(
        sorted(
            [res for res in results if int(res[0]) in THREADS],
            key=lambda x: THREADS.index(x[0]),
        )
    )
    gap = 0.1
    vlines = []
    query_value_index_map = defaultdict(list)
    hgap = 0.4
    fig, fig_axis = plt.subplots(1, 1, figsize=(3, 4))
    vlines = set()
    for thread_idx, (parallel, parallel_results) in enumerate(results):
        for category_result in parallel_results:
            for query_result_idx, query_result in list(
                enumerate(
                    sorted(
                        category_result["par_cat_result"],
                        key=lambda x: int(x["query_num"]),
                    )
                )
            ):
                current_x = query_result_idx * hgap + thread_idx * gap
                category_label = CATEGORY_MAPPING[query_result["category"]]
                print("Current query num: ", query_result["query_num"])
                query_value_index_map[str(query_result["query_num"])].append(current_x)
                fig_axis.scatter(
                    current_x,
                    query_result["total_usage_time"],
                    marker=SYSTEM_MARKERS[category_label],
                    c=SYSTEM_COLORS[category_label],
                    label=category_label,
                )
                vlines.add((parallel, current_x))
    x_mapped = [[str(ql), mean(query_value_index_map[str(ql)])] for ql in QUERIES_LIST]
    fig_axis.set_xticks([va[1] for va in x_mapped], [va[0] for va in x_mapped])
    transform = blended_transform_factory(fig_axis.transData, fig_axis.transAxes)
    for thread_value, offset in vlines:
        fig_axis.axvline(offset, linewidth=0.05, color="black")
        fig_axis.annotate(
            str(thread_value),
            xy=(offset, 0.98),
            xycoords=transform,  # arrow head position (top)
            xytext=(offset, 1.03),
            textcoords=transform,  # arrow tail position (bottom)
            arrowprops=dict(
                color="black",
                lw=0.5,
                arrowstyle="-",
                shrinkA=0,
                shrinkB=0,
            ),
            bbox=dict(
                boxstyle="square,pad=0",
                fc="none",
                ec="none",
            ),
            ha="center",
            va="bottom",
            fontsize=6,
        )
    tp_add_grid_line(fig_axis)
    legend_labels_all, handles_all = get_unique_handles_labels(fig_axis)
    fig_axis.legend(
        handles=handles_all,
        labels=legend_labels_all,
        # bbox_to_anchor=(1.10, 0.95),
        prop=dict(size=8),
    )
    fig_axis.yaxis.minorticks_on()
    fig_axis.set_xlabel("Num. Groups")
    add_arrow_label(fig_axis, "Total Time (s)", xpos=0.1)
    fig.savefig(out_dir / "sd_plot_parallel.pdf", bbox_inches="tight")


def set_num_rows(query, num_rows):
    query = query.replace("QUERY_NUM_ROWS", f"'{num_rows}'")
    return query


def fetch_results(cursor, num_rows):
    percents = [1, 5, 10, 20, 50, 100]
    sample_offset_table = "sample_offset"
    offset_query = just_read("./sd_comparison_sample_zipf.sql")
    assert offset_query is not None
    offset_query = set_num_rows(offset_query, num_rows)
    raw_percent_results = dict()
    for percent in percents:
        cursor.execute(f"drop table if exists {sample_offset_table}")
        cursor.execute(
            f"create table {sample_offset_table} (query_num bigint, experiment bigint, i_offset bigint)"
        )
        dict_values = dict(query_num=[], experiment=[], i_offset=[])
        for query in QUERIES_LIST:
            sample_count = int(math.ceil(percent * query / 100))
            random.seed(10)
            # we can only have these many offsets.
            offsets = range(query)
            all_samples = []
            for _ in range(1000):
                rand_sample = random.choices(offsets, k=sample_count)
                all_samples.append(list(rand_sample))

            for exp_id, samples in enumerate(all_samples):
                for sample in samples:
                    dict_values["experiment"].append(exp_id)
                    dict_values["query_num"].append(query)
                    dict_values["i_offset"].append(sample)
        df_data = pd.DataFrame(dict_values)
        cursor.execute(
            f"insert into {sample_offset_table} BY NAME select * from df_data"
        )
        category_results = run_query(cursor, offset_query)
        raw_percent_results[percent] = category_results
    just_write("./tmp/debug_dump.json", json.dumps(raw_percent_results, indent=4))
    return raw_percent_results


def get_breakeven(raw_percent_results):
    if raw_percent_results is None:
        raw_percent_results = json_read_file("./tmp/debug_dump.json")
    mapped_result = {
        percent: {
            (combined_result[0], combined_result[1]): {
                result_item["query_num"]: dict(
                    repeat=result_item["repeat_count"],
                    latency=result_item["phase_2_latency"],
                    index_time=(result_item["offset_independent_cost"] or 0)
                    + (result_item["phase_1_latency"] or 0),
                )
                for result_item in combined_result[2]
            }
            for (combined_result) in category_results
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

    def _reducer(prev, curr):
        category, thread_count = curr[0]
        main_result = curr[1]
        return {
            **prev,
            thread_count: ({**prev.get(thread_count, {}), category: main_result}),
        }

    per_thread_result = reduce(_reducer, category_flipped.items(), dict())
    percent_diff_results = {
        thread: {
            category: get_percent_break(thread_result["smokedduck"], category_result)
            for category, category_result in thread_result.items()
            if category not in ["smokedduck"]
        }
        for thread, thread_result in per_thread_result.items()
    }

    percent_diff_results_filtered = {
        thread: {
            category: {
                str(percent): {
                    query_num: query_result
                    for query_num, query_result in percent_result.items()
                }
                for percent, percent_result in category_result.items()
                if str(percent) in PERCENT_MAPPING
            }
            for category, category_result in percent_diff_results_per_thread.items()
        }
        for thread, percent_diff_results_per_thread in percent_diff_results.items()
        if int(thread) in THREADS
    }
    # just_write(percent_diff_dumped, json.dumps(percent_diff_results_filtered))
    just_write(
        "./tmp/debug_percent_diff.json",
        json.dumps(percent_diff_results_filtered, indent=4),
    )
    return percent_diff_results_filtered


THREADS = [1, 4, 12]
PERCENT_MAPPING = {"1": "o", "10": "P", "50": "^", "100": "s"}
PERCENT_MAPPING_COLOR = {
    "1": "#0072B2",
    "10": "#CC79A7",
    "50": "#D55E00",
    "100": "#009E73",
}


PERCENT_MAPPING_SORTED = list(
    sorted([key for key in PERCENT_MAPPING], key=lambda x: int(x[0]))
)


def plot_overall_breakeven(percent_diff_results, out_dir: Path):
    fig, fig_axis = plt.subplots(1, 1, figsize=(3, 4))
    # x_axis = np.arange(len())

    def _reducer(prev, curr):
        _percent_value, _percent_result = curr
        if str(_percent_value) not in PERCENT_MAPPING:
            return prev
        for qnum, qresult in _percent_result.items():
            prev[qnum] = {**prev.get(qnum, dict()), _percent_value: qresult}
        return prev

    percent_diff_results_mapped = {
        thread: reduce(_reducer, thread_result["traceprov"].items(), dict())
        for thread, thread_result in percent_diff_results.items()
        if int(thread) in THREADS
    }
    print(percent_diff_results_mapped)
    gap = 0.1
    vlines = []
    query_value_index_map = defaultdict(list)
    hgap = 0.4
    for thread_idx, (thread_value, thread_result) in enumerate(
        sorted(
            percent_diff_results_mapped.items(), key=lambda x: THREADS.index(int(x[0]))
        )
    ):
        for query_idx, (query_value, query_result) in enumerate(
            sorted(thread_result.items(), key=lambda x: int(x[0]))
        ):
            # y_values = [
            #     val["iter"] if isinstance(val, dict) else None for val in query_result
            # ]
            current_x = query_idx * hgap + thread_idx * gap
            query_value_index_map[query_value].append(current_x)
            for percent in PERCENT_MAPPING_SORTED:
                value = (
                    None
                    if query_result[percent] == False
                    else query_result[percent]["iter"]
                )

                fig_axis.scatter(
                    current_x,
                    value or 0,
                    marker=PERCENT_MAPPING[percent],
                    color="gray" if value is None else PERCENT_MAPPING_COLOR[percent],
                    label=percent,
                )
            vlines.append((thread_value, current_x))
            # if str()
    transform = blended_transform_factory(fig_axis.transData, fig_axis.transAxes)
    x_mapped = [[str(ql), mean(query_value_index_map[str(ql)])] for ql in QUERIES_LIST]
    fig_axis.set_xticks([va[1] for va in x_mapped], [va[0] for va in x_mapped])
    # x_ticks = [va[0] for va in x_mapped]]
    for thread_value, offset in vlines:
        fig_axis.axvline(offset, linewidth=0.05, color="black")
        fig_axis.annotate(
            str(thread_value),
            xy=(offset, 0.98),
            xycoords=transform,  # arrow head position (top)
            xytext=(offset, 1.03),
            textcoords=transform,  # arrow tail position (bottom)
            arrowprops=dict(
                color="black",
                lw=0.5,
                arrowstyle="-",
                shrinkA=0,
                shrinkB=0,
            ),
            bbox=dict(
                boxstyle="square,pad=0",
                fc="none",
                ec="none",
            ),
            ha="center",
            va="bottom",
            fontsize=6,
        )
    tp_add_grid_line(fig_axis)
    legend_labels_all, handles_all = get_unique_handles_labels(fig_axis)
    legend_labels_all_indexed = list(
        sorted(enumerate(legend_labels_all), key=lambda x: int(x[1]))
    )

    fig_axis.legend(
        handles=[handles_all[_idx[0]] for _idx in legend_labels_all_indexed],
        labels=[legend_labels_all[_idx[0]] for _idx in legend_labels_all_indexed],
        # bbox_to_anchor=(1.10, 0.95),
        prop=dict(size=8),
    )
    fig_axis.yaxis.minorticks_on()
    fig_axis.set_xlabel("Num. Groups")
    add_arrow_label(fig_axis, "Breakeven #", xpos=0.1)
    fig.savefig(out_dir / "sd_breakeven.pdf", bbox_inches="tight")


if __name__ == "__main__":
    main()

res = {
    "12": {
        "100": {
            "1": {"iter": 92, "is_less": False, "total_count": 91},
            "10": {"iter": 10, "is_less": False, "total_count": 90},
            "50": {"iter": 2, "is_less": False, "total_count": 90},
            "100": {"iter": 1, "is_less": False, "total_count": 90},
        },
        "1000": {
            "1": {"iter": 14, "is_less": False, "total_count": 139},
            "10": {"iter": 2, "is_less": False, "total_count": 139},
            "50": {"iter": 1, "is_less": False, "total_count": 139},
            "100": {"iter": 1, "is_less": False, "total_count": 139},
        },
        "10": {"1": False, "10": False, "50": False, "100": False},
    },
    "1": {
        "100": {
            "1": {"iter": 6, "is_less": False, "total_count": 5},
            "10": {"iter": 1, "is_less": False, "total_count": 5},
            "50": {"iter": 1, "is_less": False, "total_count": 5},
            "100": {"iter": 1, "is_less": False, "total_count": 5},
        },
        "1000": {
            "1": {"iter": 3, "is_less": False, "total_count": 23},
            "10": {"iter": 1, "is_less": False, "total_count": 23},
            "50": {"iter": 1, "is_less": False, "total_count": 23},
            "100": {"iter": 1, "is_less": False, "total_count": 23},
        },
        "10": {
            "1": {"iter": 4, "is_less": False, "total_count": 3},
            "10": {"iter": 4, "is_less": False, "total_count": 3},
            "50": {"iter": 1, "is_less": False, "total_count": 3},
            "100": {"iter": 1, "is_less": False, "total_count": 3},
        },
    },
    "4": {
        "10": {
            "1": {"iter": 31, "is_less": False, "total_count": 30},
            "10": {"iter": 31, "is_less": False, "total_count": 30},
            "50": {"iter": 6, "is_less": False, "total_count": 29},
            "100": {"iter": 4, "is_less": False, "total_count": 30},
        },
        "1000": {
            "1": {"iter": 9, "is_less": False, "total_count": 83},
            "10": {"iter": 1, "is_less": False, "total_count": 83},
            "50": {"iter": 1, "is_less": False, "total_count": 83},
            "100": {"iter": 1, "is_less": False, "total_count": 83},
        },
        "100": {
            "1": {"iter": 30, "is_less": False, "total_count": 29},
            "10": {"iter": 3, "is_less": False, "total_count": 29},
            "50": {"iter": 1, "is_less": False, "total_count": 29},
            "100": {"iter": 1, "is_less": False, "total_count": 29},
        },
    },
}
