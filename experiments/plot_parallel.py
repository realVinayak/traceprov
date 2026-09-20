import argparse
from pathlib import Path

from matplotlib import patches, pyplot as plt
import numpy as np

import matplotlib.lines as mlines

from plot_overview import (
    ALL_TIME_KEY,
    CATEGORY_LABEL,
    OFFSET_TIME_KEY,
    PRESENT_CATEGORIES,
    get_all,
    get_extra_predicates,
    plot_offset,
)
from traceprovpy.tools.file_utils import (
    get_breakpoint_with_min_max,
    get_min_max_categories,
)
from utils import (
    ERROR_HATCHES,
    QUERY_DIFF_COLOS,
    SYSTEM_COLORS,
    SYSTEM_MARKERS,
    DataOptions,
    PlotErrors,
    SystemLabels,
    generic_dump_legends,
    get_main_error,
    get_options_split,
    get_query_error_mapping,
    make_category_data_mapped,
)
from traceprovpy.tools.plot_utils import (
    BenchmarkPlot,
    add_arrow_label,
    get_unique_handles_labels,
    time_func_formatter,
    tp_add_grid_line,
)
from matplotlib.transforms import blended_transform_factory


def remap_data(time_results, time_key):
    thread_results = dict()
    sd_queries = set()
    for result in time_results:
        category, thread, category_thread_result = result
        if thread not in thread_results:
            thread_results[thread] = []
        thread_result = thread_results[thread]
        thread_result.append((category, category_thread_result))
        if thread > 1 and category == "smokedduck":
            sd_queries |= set(
                res["query_num"]
                for res in category_thread_result
                if res[time_key] is not None
            )
    return thread_results, sd_queries


DUCKDB_CATEGORY_MAPPING = [
    ("base", SystemLabels.base),
    ("traceprov", SystemLabels.traceprov),
    ("gprom_window_heuristics", SystemLabels.gprom),
    ("smokedduck", SystemLabels.smokedduck),
]

PG_CATEGORY_MAPPING = [
    ("base", SystemLabels.base),
    ("traceprov", SystemLabels.traceprov),
    ("gprom_window_heuristics", SystemLabels.gprom),
    ("provsql", SystemLabels.provsql),
]


def plot_legends(out_dir: Path, category_mapping):
    fig, ax = plt.subplots()
    handles_row_1 = []
    for _, system_label in category_mapping:
        patch = mlines.Line2D(
            [],
            [],
            label=system_label,
            color=SYSTEM_COLORS[system_label],
            marker=SYSTEM_MARKERS[system_label],
            markerfacecolor="none",
            markeredgecolor=SYSTEM_COLORS[system_label],
            linestyle="none",
        )
        handles_row_1.append(patch)
    generic_dump_legends(fig, ax, out_dir, handles_row_1, len(category_mapping))


def plot_thread_result(
    time_results,
    queries,
    time_key,
    options: DataOptions,
    out_dir: Path,
    break_points: list[float],
    thread_vals: list[int],
    use_rovh_mode,
):
    print("X AXIS VALUESL ", queries)
    x_axis_values = sorted(list(queries), key=lambda x: int(x))
    group_gap = 2.2
    raw_x_axis = np.arange(len(x_axis_values))
    x_axis = raw_x_axis * (1 + group_gap)
    total_time_fig, total_time_axis = plt.subplots(
        1, 1, figsize=(int((2 / 3) * len(queries)), 2)
    )
    stddev_total_time_fig, stddev_total_time_axis = plt.subplots(1, 1, figsize=(6, 2))
    width = 0.55
    MARGIN = 0.04 if len(queries) != 22 else 0.02
    total_time_axis.margins(x=MARGIN)
    y_line_axis = []
    all_y_line_axis = []
    last_positions = dict()
    threads = []
    overall_slowdown = []
    max_thread = max(thread_vals)
    max_thread_result = time_results[max_thread]
    max_thread_result_map = {
        cat: make_category_data_mapped(cat_result)
        for cat, cat_result in max_thread_result
    }

    def maybe_slowdown(value):
        if use_rovh_mode:
            return 100 * (value - 1)
        return value

    traceprov_slowdown = [
        (
            q,
            (
                maybe_slowdown(
                    max_thread_result_map["traceprov"][q][time_key]
                    / max_thread_result_map["base"][q][time_key]
                )
            ),
        )
        for q in x_axis_values
    ]
    min_max_slowdown, min_max_pairs, broken_categories = get_min_max_categories(
        traceprov_slowdown, break_points
    )
    min_max_labels = get_breakpoint_with_min_max(min_max_pairs, use_rovh_mode)

    x_axis_values = [q for cat in broken_categories for q in cat]
    for thread_idx, (thread, thread_category_result) in list(
        enumerate(list(sorted(time_results.items(), key=lambda x: x[0])))
    ):
        threads.append(thread)
        results_filtered = sorted(
            [
                result
                for result in thread_category_result
                if result[0] in PRESENT_CATEGORIES
            ],
            key=lambda x: PRESENT_CATEGORIES.index(x[0]),
        )
        # no need to sort data in this case..

        for category_idx, (category_key, category_data) in enumerate(results_filtered):
            category_data_mapped = make_category_data_mapped(category_data)
            query_errors = [
                (
                    (q, get_main_error(category_data_mapped[q]["fail_reason"], options))
                    if (q in category_data_mapped)
                    else (q, PlotErrors.not_available)
                )
                for q in x_axis_values
            ]
            query_with_errors_mapped = get_query_error_mapping(query_errors)
            # print(thread, category_key, category_data_mapped, query_with_errors_mapped)
            query_without_errors = query_with_errors_mapped[None]

            def _get_values(in_key):
                return [
                    (
                        None
                        if q not in query_without_errors
                        else category_data_mapped[q][in_key]
                    )
                    for q in x_axis_values
                ]

            total_time_values = _get_values(time_key)
            stddev_total_time_values = _get_values("total_time_std_ratio")
            x_axis_shifted = x_axis + width * thread_idx
            true_cat_label = CATEGORY_LABEL[category_key]
            if category_key == "traceprov":
                size = 50
            else:
                # print(category_key, x_axis_shifted, total_time_values)
                size = 40
            # if thread_idx == 0 or thread_idx == len(time_results) - 1:
            #     size = 10
            # else:
            #     size = 10
            print("TIME VALUES: ", total_time_values)
            return_value = total_time_axis.scatter(
                x_axis_shifted,
                total_time_values,
                label=true_cat_label,
                marker=SYSTEM_MARKERS[true_cat_label],
                facecolors="none",
                edgecolors=SYSTEM_COLORS[true_cat_label],
                linewidths=1.5,
                s=size,
            )
            return_value_2 = stddev_total_time_axis.scatter(
                x_axis_shifted,
                stddev_total_time_values,
                label=true_cat_label,
                marker=SYSTEM_MARKERS[true_cat_label],
                s=size,
                facecolors="none",
                edgecolors=SYSTEM_COLORS[true_cat_label],
                linewidths=1.5,
            )
            last_positions[category_key] = return_value
            if category_idx == 0:
                for offsets in return_value.get_offsets():
                    print("OFFSETS: ", offsets[0], offsets[1])
                    vline = total_time_axis.axvline(
                        offsets[0], linewidth=0.05, color="black"
                    )
                for offsets in return_value_2.get_offsets():
                    # print("OFFSETS: ", offsets[0], offsets[1])
                    stddev_total_time_axis.axvline(
                        offsets[0], linewidth=0.05, color="black"
                    )
                print(vline.get_xydata())
                y_line_axis.append(
                    (return_value.get_offsets()[0][0], vline.get_ydata(False)[0])
                )
            # for error, error_queries in query_with_errors_mapped.items():
            #     if error is None:
            #         continue
            #     error_queries_idx = list(map(x_axis_values.index, error_queries))
            #     scatter_arg = dict(
            #         x=[x_axis_shifted[idx] for idx in error_queries_idx],
            #         height=0,
            #         label=error,
            #         color="lightgray",
            #         edgecolor=SYSTEM_COLORS[true_cat_label],
            #         hatch=ERROR_HATCHES[error],
            #         marker=SYSTEM_MARKERS[true_cat_label],
            #     )

        # if thread_idx == (len(time_results) - 1):

    total_time_axis.set_xticks(
        x_axis + width * ((len(time_results) - 1) / 2),
        x_axis_values,
    )
    stddev_total_time_axis.set_xticks(
        x_axis + width * ((len(time_results) - 1) / 2),
        x_axis_values,
    )
    total_time_axis.set_yscale("log")
    # legend_labels_all, handles_all = get_unique_handles_labels(total_time_axis)
    # total_time_axis.legend(
    #     handles=handles_all,
    #     labels=legend_labels_all,
    #     # bbox_to_anchor=(1.10, 0.95),
    #     prop=dict(size=8),
    # )
    tp_add_grid_line(total_time_axis)
    transform = blended_transform_factory(
        total_time_axis.transData, total_time_axis.transAxes
    )
    running = 0
    for _idx, query_category in enumerate(broken_categories):
        old_running = running
        running += len(query_category)
        if _idx < len(broken_categories) - 1:
            total_time_axis.axvline(
                x=(1 + group_gap) * (running) - width,
                linestyle="solid",
                color="red",
                linewidth=2.5,
            )
        new_running = running
        middle = int((old_running + new_running) / 2)
        total_time_axis.text(
            middle * (1 + group_gap) - width,
            1.1,
            min_max_labels[_idx],
            transform=transform,
            ha="center",
            va="bottom",
            fontsize=9,
            ma="center",
        )
    for _idx, points in enumerate(y_line_axis):
        thread_value = threads[_idx]
        print(points)
        original_end = points[0]
        if _idx == len(y_line_axis) - 1:
            end = points[0] - 0.15
        else:
            end = points[0] - 0.5
        total_time_axis.annotate(
            str(thread_value),
            xy=(points[0], 0.98),
            xycoords=transform,  # arrow head position (top)
            xytext=(end, 1.05),
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
            ha="right",
            va="bottom",
            fontsize=6,
        )
        if thread_value in (1, 4, 10):
            for shifted in range(1, len(queries)):
                end = original_end + (1 + group_gap) * shifted
                total_time_axis.annotate(
                    str(thread_value),
                    xy=(end, 0.98),
                    xycoords=transform,  # arrow head position (top)
                    xytext=(end, 1.03),
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
        # add_arrow_label(total_time_axis, threads[_idx], points[0], 1.1, transform)

        # total_time_axis.annotate(
        #     f"{threads[_idx]}",
        #     (points[0], 1.002),
        #     rotation=0,
        #     fontsize=8,
        #     xycoords=transform,
        # )
    # scalability_axis.margins(0.0)
    x_axis_augmented = np.arange(len(x_axis_values) + 1) * (1 + group_gap)
    x_axis_augmented = [max(0, val) for val in x_axis_augmented]
    _thread_end = None
    last_end = None
    for _idx, x_span in enumerate(x_axis_augmented[:-1]):
        _start = x_span
        _thread_end = x_span + (len(time_results) - 1) * width
        if _idx == 0:
            _start -= 5 * MARGIN
        else:
            _start = (_start + _thread_end) / 2
        if last_end is None:
            last_end = _start
        current_end = (_thread_end + x_axis_augmented[_idx + 1]) / 2
        total_time_axis.axvspan(
            last_end,
            current_end,
            color=QUERY_DIFF_COLOS[_idx % 2],
            zorder=0,
        )
        last_end = current_end
        total_time_axis.axvline(current_end, color="black", linewidth=1.5)

    total_time_axis.yaxis.set_major_formatter(time_func_formatter())
    total_time_axis.set_xlabel(
        rf"Query (TraceProv slowdown @ {max_thread} threads $\longrightarrow$)"
    )
    add_arrow_label(total_time_axis, "Total Time (s)", 0.02)
    total_time_fig.savefig(out_dir / "scalability_thread.pdf", bbox_inches="tight")
    stddev_total_time_fig.savefig(
        out_dir / "scalability_thread_total_time_std.pdf", bbox_inches="tight"
    )


DUCKDB_PARALLEL = [1, 2, 4, 8, 10]

POSTGRES_PARALLEL = [1, 3, 5, 9, 11]


def main():
    plot_context = BenchmarkPlot("plot_parallel", ignore_args=True)
    plot_context.parser.add_argument("--db", required=True)
    plot_context.parser.add_argument(
        "--postgres_tpch_dir", required=False, default=("../postgres/benchmark/tpch/")
    )
    plot_context.parser.add_argument(
        "--duckdb_tpch_dir", required=False, default=("../duckdb/benchmark/tpch/")
    )
    plot_context.parser.add_argument(
        "--restrict_sd", action=argparse.BooleanOptionalAction, default=False
    )
    plot_context.parser.add_argument(
        "--relative", action=argparse.BooleanOptionalAction, default=False
    )
    plot_context.parser.add_argument("--break_points", nargs="*", default=[])
    parsed = plot_context.parser.parse_args()
    db_options = get_options_split(parsed)
    out_dir = plot_context.add_timestamp()
    extra_predicates = get_extra_predicates(parsed, db_options)
    extra_predicates = f"({extra_predicates}) and fail_reason = ''"
    if db_options.get_db_name() == "postgres":
        thread_vals = POSTGRES_PARALLEL
        category_mapping = PG_CATEGORY_MAPPING
    else:
        thread_vals = DUCKDB_PARALLEL
        category_mapping = DUCKDB_CATEGORY_MAPPING
    if db_options.mode == "all":
        time_results, _ = get_all(parsed, None, extra_predicates, None)
        time_key = ALL_TIME_KEY
    else:
        time_results, _ = plot_offset(parsed, None, db_options, extra_predicates, None)
        time_key = OFFSET_TIME_KEY
    remap_time_results, sd_queries = remap_data(time_results, time_key)
    remap_time_results = {
        thread: thread_result
        for (thread, thread_result) in remap_time_results.items()
        if thread in thread_vals
    }
    if not parsed.restrict_sd:
        sd_queries = set(map(str, range(1, 23)))
    plot_thread_result(
        remap_time_results,
        sd_queries,
        time_key,
        db_options,
        out_dir,
        list([float(bp) for bp in parsed.break_points]),
        thread_vals,
        parsed.relative,
    )
    plot_legends(out_dir, category_mapping)
    # print(all_results)


if __name__ == "__main__":
    main()
