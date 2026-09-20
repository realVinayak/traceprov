import argparse
from functools import reduce
import json
import math
from pathlib import Path
import random
import pandas as pd
from matplotlib import pyplot as plt, ticker
import numpy as np

from traceprovpy.tools.traceprov_sd_layer import (
    add_tp_sd_eq_layer_comp,
    get_eq_queries,
    set_all_tp_sd_comp,
)
from utils import (
    ERROR_HATCHES,
    QUERY_LABEL,
    SYSTEM_COLORS,
    DataOptions,
    PlotErrors,
    SystemLabels,
    get_main_error,
    get_query_error_mapping,
    make_category_data_mapped,
)
from traceprovpy.tools.file_utils import (
    get_breakpoint_labels,
    get_nice_num,
    get_slowdown_cats,
    json_read_file,
    just_read,
    just_write,
    run_query,
)
from traceprovpy.tools.plot_utils import (
    BenchmarkPlot,
    add_arrow_label,
    make_list_query,
    replace_extra_predicate,
    tp_add_grid_line,
)
from matplotlib.transforms import blended_transform_factory

import duckdb

CATEGORY_MAPPING = [
    ("traceprov", SystemLabels.traceprov),
    ("smokedduck", SystemLabels.smokedduck),
]

PRESENT_CATEGORIES = [cat[0] for cat in CATEGORY_MAPPING]

CATEGORY_LABEL = {cat[0]: cat[1] for cat in CATEGORY_MAPPING}


def plot_main(
    raw_offset_capture_results,
    backtrace_index_results,
    out_dir: Path,
    options: DataOptions,
):
    fig, (fig_axis, infer_time_axis, setup_cost_axis) = plt.subplots(
        3, 1, figsize=(8, 5)
    )
    fig.subplots_adjust(hspace=0.3)
    offset_capture_results = [
        res for res in raw_offset_capture_results if res[0] in PRESENT_CATEGORIES
    ]
    offset_capture_results = sorted(
        offset_capture_results, key=lambda x: PRESENT_CATEGORIES.index(x[0])
    )
    tp_category_data = offset_capture_results[PRESENT_CATEGORIES.index("traceprov")][1]
    # print(tp_category_data)
    relative_ovh_data = [
        (item["query_num"], item["phase_1_relative_overhead"])
        for item in tp_category_data
    ]
    break_points = [10, 20]
    rovh_cats = get_slowdown_cats(relative_ovh_data, break_points)
    breakpoint_labels = get_breakpoint_labels(break_points)
    x_axis_combined = [cell for node in rovh_cats for cell in node]
    x_axis_values = list(map(str, x_axis_combined))
    x_axis = np.arange(len(x_axis_values))
    width = 0.3
    time_key = "phase_1_profile_latency_time"
    pending_bars = []
    for category_idx, (category, category_data) in enumerate(offset_capture_results):
        category_data_mapped = make_category_data_mapped(category_data)
        category_backtrace_index = backtrace_index_results[category]
        query_errors = [
            (
                (q, get_main_error(category_data_mapped[q]["fail_reason"], options))
                if (q in category_data_mapped)
                else (q, PlotErrors.not_available)
            )
            for q in x_axis_values
        ]
        query_with_errors_mapped = get_query_error_mapping(query_errors)
        query_without_errors = query_with_errors_mapped[None]
        total_time_values = [
            (0 if q not in query_without_errors else category_data_mapped[q][time_key])
            for q in x_axis_values
        ]
        backtrace_time_values = [
            (
                0
                if q not in query_without_errors
                else category_backtrace_index[q]["phase_2_latency"]
            )
            for q in x_axis_values
        ]
        offset_independent_cost = [
            (
                0
                if q not in query_without_errors
                else category_backtrace_index[q]["offset_independent_cost"]
            )
            for q in x_axis_values
        ]
        print(category, total_time_values, query_without_errors)
        x_axis_shifted = x_axis + width * category_idx
        fig_axis.bar(
            x_axis_shifted,
            total_time_values,
            width=width,
            label=CATEGORY_LABEL[category],
            color=SYSTEM_COLORS[CATEGORY_LABEL[category]],
        )
        infer_time_axis.bar(
            x_axis_shifted,
            backtrace_time_values,
            width=width,
            label=CATEGORY_LABEL[category],
            color=SYSTEM_COLORS[CATEGORY_LABEL[category]],
        )
        setup_cost_axis.bar(
            x_axis_shifted,
            offset_independent_cost,
            width=width,
            label=CATEGORY_LABEL[category],
            color=SYSTEM_COLORS[CATEGORY_LABEL[category]],
        )
        for error, error_queries in query_with_errors_mapped.items():
            if error is None:
                continue
            error_queries_idx = list(map(x_axis_values.index, error_queries))
            bar_dict = dict(
                x=[x_axis_shifted[idx] for idx in error_queries_idx],
                height=0,
                width=width,
                label=error,
                color="lightgray",
                edgecolor=SYSTEM_COLORS[CATEGORY_LABEL[category]],
                hatch=ERROR_HATCHES[error],
            )
            pending_bars.append(bar_dict)
    all_axes = (fig_axis, infer_time_axis, setup_cost_axis)

    def _do_for_all(func):
        for a in all_axes:
            func(a)

    for curr_axi in all_axes:
        for bar in pending_bars:
            bar["height"] = 0.85
            bar["transform"] = blended_transform_factory(
                curr_axi.transData, curr_axi.transAxes
            )
            _do_for_all(lambda axis: axis.bar(**bar))

    _do_for_all(lambda axis: axis.margins(x=0.0))
    _do_for_all(tp_add_grid_line)
    colors = ["#d6d6d6", "white"]
    trans = blended_transform_factory(fig_axis.transData, fig_axis.transAxes)
    # categories_labels_rovh = ["<= 10%", "> 10% & <= 20%", f"<= {max_rovh}"]
    running = 0

    for _idx, query_category in enumerate(rovh_cats):
        old_running = running
        running += len(query_category)

        if _idx < len(rovh_cats) - 1:
            _do_for_all(
                lambda axis: axis.axvline(
                    x=running - width - 0.00,
                    linestyle="solid",
                    color="black",
                    linewidth=1,
                )
            )
        new_running = running
        middle = int((old_running + new_running) / 2)
        fig_axis.text(
            middle,
            1.02,
            breakpoint_labels[_idx],
            transform=trans,
            ha="center",
            va="bottom",
            fontsize=9,
            ma="center",
        )
        start_ = old_running - width
        if _idx == 0:
            start_ -= 0.1
        _do_for_all(
            lambda axis: axis.axvspan(
                start_, new_running - width, color=colors[_idx % 2], zorder=0
            )
        )
    _do_for_all(
        lambda fig: fig.set_xticks(
            x_axis - width / 2 + (width * (len(offset_capture_results) / 2)),
            x_axis_values,
        )
    )
    _do_for_all(lambda fig: fig.set_yscale("log"))
    setup_cost_axis.set_xlabel(QUERY_LABEL)

    all_axes[0].set_ylabel("Capture (s)")
    all_axes[1].set_ylabel("Backtrace (ms)")
    all_axes[2].set_ylabel("Setup (ms)")
    _format_wrapped = lambda x, _: get_nice_num(x)

    all_axes[0].yaxis.set_major_formatter(ticker.FuncFormatter(_format_wrapped))

    all_axes[1].set_yticks([10 ** (-4), 10 ** (-3), 10 ** (-2)], ["0.1", "1", "10"])
    all_axes[2].set_yticks([10 ** (-4), 10 ** (-3), 10 ** (-0)], ["0.1", "1", "1000"])

    # all_axes[1].yaxis.set_major_formatter(ticker.FuncFormatter(_format_wrapped))

    fig.savefig(out_dir / "sd_comparison_offset.pdf", bbox_inches="tight")


def replace_table(orig_sql: str, table: str):
    key = "current_sample_table"
    assert key in orig_sql
    return orig_sql.replace(key, table)


def get_base_counts(cursor):
    query = """
    select
        query_num,
        any_value(phase_1_row_count) as base_count
    from
        data_all.dumped_versioned_filtered
    where
        "parallel" = 1
        and category = 'base'
    group by
        query_num;
    """
    base_counts_flat = run_query(cursor, query)
    base_count = {q: b_count for (q, b_count) in base_counts_flat}
    return base_count


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
    count = index_diff / latency_diff
    if count < 0:
        if is_less:
            return False
        count = 0
    return dict(
        iter=math.ceil(count),
        is_less=is_less,
        total_count=int(in_dict["record_count"] * count),
    )


def plot_overall_breakeven(raw_percent_results, query_row_count_map, out_dir: Path):
    percent_diff_dumped = "./tmp/percent_diff.json"
    if raw_percent_results is not None:
        mapped_result = {
            percent: {
                category: {
                    result_item["query_num"]: dict(
                        repeat=result_item["repeat_count"],
                        latency=result_item["phase_2_latency"],
                        index_time=(result_item["offset_independent_cost"] or 0)
                        + (result_item["phase_1_latency"] or 0),
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
        sd_result = category_flipped["smokedduck"]
        percent_diff_results = {
            category: get_percent_break(sd_result, category_result)
            for (category, category_result) in category_flipped.items()
            if category not in ["smokedduck"]
        }
        percent_diff_results_filtered = {
            category: {
                str(percent): {
                    query_num: query_result
                    for query_num, query_result in percent_result.items()
                    if query_result != False and ((not query_result["is_less"]))
                }
                for percent, percent_result in category_result.items()
            }
            for category, category_result in percent_diff_results.items()
        }
        just_write(percent_diff_dumped, json.dumps(percent_diff_results_filtered))
    else:
        percent_diff_results_filtered = json_read_file(percent_diff_dumped)
    _query_num_available = [
        set(percent_result.keys())
        for _, category_result in percent_diff_results_filtered.items()
        for __, percent_result in category_result.items()
    ]
    query_available = reduce(
        lambda prev, curr: prev | curr, _query_num_available, set()
    )
    # x_axis_values = sorted(list(query_available), key=lambda q: int(q))
    # x_axis = np.arange(len(x_axis_values))
    percent_breakeven_fig, percent_breakeven_axis = plt.subplots(1, 1, figsize=(6.5, 3))
    width = 0.14
    group_gap = 0.01
    for category, category_result in percent_diff_results_filtered.items():
        print(category_result)
        values_at_100 = category_result["100"]
        query_sort_order = sorted(
            values_at_100.keys(), key=lambda _key: values_at_100[_key]["iter"]
        )
        x_axis_values = [q for q in query_sort_order if q in query_available]
        x_axis = np.arange(len(x_axis_values))
        print(x_axis_values)
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
            # percent_breakeven_axis.bar(
            #     x_axis + width * percent_idx,
            #     [-1 * int(y) for y in y_values_count_less],
            #     width=width,
            #     hatch="///",
            #     color=BenchmarkPlot.colors[percent_idx],
            #     alpha=0.99,
            # )
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

    Y_POS = 0.9
    X_START = 0.65
    trans2 = blended_transform_factory(
        percent_breakeven_axis.transAxes, percent_breakeven_axis.transAxes
    )
    percent_breakeven_axis.annotate(
        "",
        xy=(X_START + 0.15, Y_POS),
        xycoords=trans2,  # arrow head position (top)
        xytext=(X_START, Y_POS),
        textcoords=trans2,  # arrow tail position (bottom)
        arrowprops=dict(arrowstyle="->", color="black", lw=1.5),
    )
    percent_breakeven_axis.text(
        X_START,
        Y_POS,
        "Decreasing index benefit",
        transform=percent_breakeven_axis.transAxes,
        ha="right",
        va="center",
        fontsize=12,
    )

    percent_breakeven_axis.set_xticks(x_axis + width * (percent_idx / 2), x_axis_values)
    percent_breakeven_axis.set_yscale("log")
    yticks = []
    ytick_labels = []
    percent_breakeven_axis.axhline(y=0, color="black", linestyle="solid")
    percent_breakeven_axis.axhline(y=1, color="black", linestyle="--", linewidth=0.7)
    percent_breakeven_axis.set_xlabel("Query", fontdict={"fontsize": 12})
    percent_breakeven_axis.margins(x=0.01)
    tp_add_grid_line(percent_breakeven_axis)

    percent_breakeven_axis.legend(prop=dict(size=11.5), labelspacing=0.3)
    # percent_breakeven_axis.text(
    #     -0.05,
    #     0.75,
    #     "Breakeven \n Iteration",
    #     transform=percent_breakeven_axis.transAxes,
    #     ha="right",
    #     va="bottom",
    #     fontsize=10,
    #     horizontalalignment="center",
    #     verticalalignment="center",
    #     ma="center",
    # )
    # percent_breakeven_axis.text(
    #     -0.05,
    #     0.25,
    #     "Total Request \n Count",
    #     transform=percent_breakeven_axis.transAxes,
    #     ha="right",
    #     va="top",
    #     fontsize=10,
    #     horizontalalignment="center",
    #     verticalalignment="center",
    #     ma="center",
    # )
    percent_breakeven_axis.set_ylabel(
        "Breakeven Iteration", fontdict={"fontsize": 12}
    )  # leave main label empty, or use a generic axis title
    # for tick_labels in percent_breakeven_axis.get_yticklabels():
    #     yticks.append(tick_labels.get_position()[1])
    #     print(yticks)
    #     value = get_nice_num(int(tick_labels.get_text()))
    #     ytick_labels.append(value)
    print(yticks, ytick_labels)
    _format_wrapped = lambda x, _: get_nice_num(x)
    # percent_breakeven_axis.set_yticks(yticks, ytick_labels)
    percent_breakeven_axis.yaxis.set_major_formatter(
        ticker.FuncFormatter(_format_wrapped)
    )
    # percent_breakeven_axis.
    percent_breakeven_axis.tick_params(
        axis="x", labelsize=12
    )  # Set x-axis labels to size 12
    percent_breakeven_axis.tick_params(
        axis="y", labelsize=12
    )  # Set y-axis labels to size 12

    percent_breakeven_axis.yaxis.set_minor_locator(
        ticker.LogLocator(
            base=10.0,
            subs=[2, 3, 4, 5, 6, 7, 8, 9],
            # linthresh=percent_breakeven_axis.yaxis.get_transform().linthresh,
        )
    )
    percent_breakeven_axis.tick_params(axis="y", which="minor", left=True)

    # percent_breakeven_axis.set_title(
    #     "Breakeven Iteration & \n Total Request Count per Query"
    # )
    # percent_breakeven_axis.set_xticklabels(x_axis)
    percent_breakeven_fig.savefig(
        out_dir / "percent_breakeven.pdf", bbox_inches="tight"
    )


def fetch_backtrace_index_times(cursor, use_cached_sample: bool):
    orig_sql = add_tp_sd_eq_layer_comp(just_read("./sd_comparison_index_time.sql"))
    base_sql = replace_table(orig_sql, "dumped_versioned_filtered")
    cursor.execute(base_sql)
    results = cursor.fetchall()
    results_nice = {
        category: {res["query_num"]: res for res in category_results}
        for category, category_results in results
    }
    percents = [1, 5, 10, 20, 50, 100]
    sample_offset_table = "data_all.sample_offset"
    query_row_count_result_map = get_base_counts(cursor)
    queries = [q for q in list(map(str, range(1, 23))) if q not in ["16"]]
    offset_orig_query = add_tp_sd_eq_layer_comp(
        just_read("./sd_comparison_index_time_sample.sql")
    )
    offset_query = replace_table(offset_orig_query, "dumped_versioned_filtered")
    if use_cached_sample:
        return results_nice, None, query_row_count_result_map
    raw_percent_results = dict()
    for percent in percents:
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
    return results_nice, raw_percent_results, query_row_count_result_map


def plot_offset_all_layer_backtrace(cursor, out_dir: Path):
    orig_sql = set_all_tp_sd_comp(just_read("./sd_comparison_index_time.sql"))
    base_sql = replace_table(orig_sql, "dumped_versioned_filtered")
    cursor.execute(base_sql)
    results = cursor.fetchall()
    off_all_q_fig, off_all_q_axis = plt.subplots(1, 1, figsize=(3, 2))
    offset_capture_results = [res for res in results if res[0] in PRESENT_CATEGORIES]
    offset_capture_results = sorted(
        offset_capture_results, key=lambda x: PRESENT_CATEGORIES.index(x[0])
    )
    x_axis_values = list(map(str, get_eq_queries()))
    x_axis = np.arange(len(x_axis_values))
    width = 0.3
    for category_idx, (category, category_data) in enumerate(offset_capture_results):
        category_data_mapped = make_category_data_mapped(category_data)
        backtrace_time_values = [
            category_data_mapped[q]["phase_2_latency"] for q in x_axis_values
        ]
        x_axis_shifted = x_axis + width * category_idx
        off_all_q_axis.bar(
            x_axis_shifted,
            backtrace_time_values,
            width=width,
            label=CATEGORY_LABEL[category],
            color=SYSTEM_COLORS[CATEGORY_LABEL[category]],
        )

    off_all_q_axis.set_xticks(
        x_axis - (width / 2) + width * (len(results) / 2), x_axis_values
    )
    off_all_q_axis.margins(x=0.01)
    off_all_q_axis.set_yscale("log")
    tp_add_grid_line(off_all_q_axis)
    add_arrow_label(off_all_q_axis, "All Backtrace (ms)", xpos=0.2)
    # def _format_wrapped(val):
    #     adj = val*1000

    # all_axes[0].yaxis.set_major_formatter(ticker.FuncFormatter(_format_wrapped))
    # off_all_q_axis.set_yticks([10 ** (-4), 10 ** (-3), 10 ** (-0)], ["0.1", "1", "1000"])
    off_all_q_fig.savefig(
        out_dir / "duckdb_lq_all_layer_prov_raw.pdf", bbox_inches="tight"
    )
    off_all_q_axis.set_yticks(
        [10 ** (-5), 10 ** (-4), 10 ** (-3), (10) ** (-2), (10) ** (-1)],
        ["0.01", "0.1", "1", "10", "100"],
    )
    off_all_q_fig.savefig(
        out_dir / "duckdb_lq_all_layer_prov_axis.pdf", bbox_inches="tight"
    )


def plot_all_mode_backtrace(cursor, out_dir: Path):
    overview_query = just_read("./overview_all_query.sql")
    overview_query = make_list_query(overview_query, "true", parallel=1)
    overview_query = overview_query.replace(
        "dumped_versioned_filtered", "data_all.dumped_versioned_filtered"
    )
    cursor.execute(overview_query)
    results = cursor.fetchall()
    x_axis_values = list(map(str, range(1, 23)))
    x_axis = np.arange(len(x_axis_values))
    phase_2_fig, phase_2_axis = plt.subplots(1, 1, figsize=(4, 1.75))
    offset_capture_results = [res for res in results if res[0] in PRESENT_CATEGORIES]
    offset_capture_results = sorted(
        offset_capture_results, key=lambda x: PRESENT_CATEGORIES.index(x[0])
    )
    markers = ["o", "^"]
    for category_idx, (category, category_data) in enumerate(offset_capture_results):
        category_data_mapped = make_category_data_mapped(category_data)
        backtrace_time_values = [
            category_data_mapped[q]["backtrace_time_median"] for q in x_axis_values
        ]
        phase_2_axis.scatter(
            x_axis,
            backtrace_time_values,
            # width=width,
            label=CATEGORY_LABEL[category],
            color=SYSTEM_COLORS[CATEGORY_LABEL[category]],
            marker=markers[category_idx],
        )
    # phase_2_axis.set_ylim(10 ** (-5))
    phase_2_axis.margins(0.04)
    phase_2_axis.minorticks_on()
    phase_2_axis.legend(
        ncols=2,
        fontsize="small",
        columnspacing=1,
        handletextpad=0.1,
        loc="lower right",
        bbox_to_anchor=(1, 1),
    )
    phase_2_axis.set_yscale("log")
    x_axis_nice = np.array([0, 4, 9, 14, 19, 21])
    x_axis_label_nice = ["1", "5", "10", "15", "20", "22"]
    phase_2_axis.set_xticks(x_axis_nice, x_axis_label_nice)
    plt.grid(which="major", color="black", linestyle="-", linewidth=0.8, axis="x")
    plt.grid(which="minor", color="black", linestyle="-", linewidth=0.8, axis="x")
    plt.grid(which="major", color="gray", linestyle="-", linewidth=0.5, axis="y")
    # plt.grid(which='minor', color='gray', linestyle='-', linewidth=0.8, axis='y')
    # phase_2_fig.legend()
    add_arrow_label(phase_2_axis, "Backtrace (s)", 0.0)
    phase_2_fig.savefig(out_dir / f"sd_backtrace_all.pdf", bbox_inches="tight")


def main():
    plot_context = BenchmarkPlot("plot_sd_comparison", ignore_args=True)
    plot_context.parser.add_argument("--db_dir", required=True)
    plot_context.parser.add_argument("--sf", required=True, type=int)
    plot_context.parser.add_argument(
        "--use_cache_sample",
        required=False,
        action=argparse.BooleanOptionalAction,
        default=False,
    )
    parsed = plot_context.parser.parse_args()

    db_option = lambda _mode: DataOptions(
        sf=f"sf_{parsed.sf}", db_name="duckdb", mode=_mode
    )
    all_db_path = Path(parsed.db_dir) / db_option("all").to_db()
    offset_db_path = Path(parsed.db_dir) / db_option("offset").to_db()
    parsed.db = offset_db_path.name
    out_dir = plot_context.add_timestamp()
    assert all_db_path.exists(), f"expected {all_db_path} to exist!"
    assert offset_db_path.exists(), f"expected {offset_db_path} to exist!"

    conn = duckdb.connect()
    conn.execute(f"attach '{all_db_path.as_posix()}' as data_all")
    conn.execute(f"attach '{offset_db_path.as_posix()}' as data_offset")
    cursor = conn.cursor()
    sd_comparison_query = just_read("./sd_comparison_query.sql")
    cursor.execute(sd_comparison_query)
    results = list(cursor.fetchall())
    backtrace_index_times, raw_percent_results, base_counts = (
        fetch_backtrace_index_times(cursor, parsed.use_cache_sample)
    )

    plot_main(results, backtrace_index_times, out_dir, db_option("offset"))
    plot_overall_breakeven(raw_percent_results, base_counts, out_dir)
    plot_offset_all_layer_backtrace(cursor, out_dir)
    plot_all_mode_backtrace(cursor, out_dir)
    # print(results)
    cursor.close()


if __name__ == "__main__":
    main()
