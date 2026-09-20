from pathlib import Path

from matplotlib import pyplot as plt
import numpy as np

from utils import (
    ERROR_HATCHES,
    SYSTEM_COLORS,
    DataOptions,
    PlotErrors,
    SystemLabels,
    get_main_error,
    get_query_error_mapping,
)
from traceprovpy.tools.file_utils import just_read, run_query
from traceprovpy.tools.plot_utils import (
    BenchmarkPlot,
    add_arrow_label,
    make_list_query,
    tp_add_grid_line,
)
import duckdb
from matplotlib.transforms import blended_transform_factory

FACTOR_ID = ("query_num_factor_value", "query_num_is_factor")
JOIN_ID = ("query_num_num_rows", "query_num_join_count")

CATEGORY_MAPPING = [
    ("base", SystemLabels.base),
    ("traceprov", SystemLabels.traceprov),
    ("gprom", SystemLabels.gprom),
    ("smokedduck", SystemLabels.smokedduck),
    ("muller", SystemLabels.muller),
    ("provsql", SystemLabels.provsql),
]

PRESENT_CATEGORIES = [cat[0] for cat in CATEGORY_MAPPING]

CATEGORY_LABEL = {cat[0]: cat[1] for cat in CATEGORY_MAPPING}


def plot_factor(all_results, out_dir, backend_system, time_key):
    mock_data_options = DataOptions("0", backend_system, "all")
    all_results_sorted = list(
        sorted(all_results, key=lambda x: PRESENT_CATEGORIES.index(x[0]))
    )
    total_time_fig, total_time_axis = plt.subplots(2, 1, figsize=(3, 2), sharex=True)
    x_axis_values = [8, 16, 32, 64, 128]
    #     4 │
    # │                    128 │
    # │                      8 │
    # │                     64 │
    # │                      2 │
    # │                     16 │
    # │                     32
    x_axis = np.arange(len(x_axis_values))
    width = 0.2
    bar_width = width
    xticks = x_axis - (width / 2) + (len(all_results_sorted) * (width / 2))
    for category_idx, (category_key, _, category_result) in enumerate(
        all_results_sorted
    ):

        def plot_result(factor_value, current_axis, needs_log=True):
            pending_bars = []
            factor_results = [
                res
                for res in category_result
                if res["query_num_is_factor"] == factor_value
            ]
            factor_results_mapped = {
                x["query_num_factor_value"]: x for x in factor_results
            }
            MARGIN = 0.0
            current_axis.margins(x=MARGIN)
            query_errors = list(
                [
                    (
                        (
                            q,
                            get_main_error(
                                factor_results_mapped[q]["fail_reason"],
                                mock_data_options,
                            ),
                        )
                        if (q in factor_results_mapped)
                        else (q, PlotErrors.not_available)
                    )
                    for q in x_axis_values
                ]
            )
            query_with_errors_mapped = get_query_error_mapping(query_errors)
            query_without_errors = query_with_errors_mapped[None]
            total_time_values = [
                (
                    0
                    if q not in query_without_errors
                    else factor_results_mapped[q][time_key]
                )
                for q in x_axis_values
            ]
            x_axis_shifted = x_axis + width * category_idx
            current_axis.bar(
                x_axis_shifted,
                total_time_values,
                width=bar_width,
                label=CATEGORY_LABEL[category_key],
                color=SYSTEM_COLORS[CATEGORY_LABEL[category_key]],
            )
            for error, error_queries in query_with_errors_mapped.items():
                if error is None:
                    continue
                error_queries_idx = list(map(x_axis_values.index, error_queries))
                bar_dict = dict(
                    x=[x_axis_shifted[idx] for idx in error_queries_idx],
                    height=0,
                    width=bar_width,
                    label=error,
                    color="lightgray",
                    edgecolor=SYSTEM_COLORS[CATEGORY_LABEL[category_key]],
                    hatch=ERROR_HATCHES[error],
                )
                pending_bars.append(bar_dict)

            xdata_yscale_t = blended_transform_factory(
                current_axis.transData, current_axis.transAxes
            )
            for bar in pending_bars:
                bar["height"] = 0.85
                bar["transform"] = xdata_yscale_t
                current_axis.bar(**bar)

            if needs_log:
                current_axis.set_yscale("log")
            tp_add_grid_line(current_axis)

        plot_result(True, total_time_axis[0], backend_system != "duckdb")
        plot_result(False, total_time_axis[1])

    total_time_axis[0].set_xticks(xticks)
    x_labels = [str(round(((val / 256) ** 3) * 100, 3)) for val in x_axis_values]
    total_time_axis[1].set_xticks(xticks, x_labels)
    add_arrow_label(
        total_time_axis[0], "Total Time (s)", 0.0, ypos=1.25, ypos_start=1.2
    )
    total_time_axis[1].set_xlabel("Selectivity (%)")
    total_time_fig.savefig(
        out_dir / f"factor_{backend_system}.pdf",
        bbox_inches="tight",
    )


def plot_join(all_results, out_dir, backend_system, time_key):
    mock_data_options = DataOptions("0", backend_system, "all")
    num_rows = [1_000, 5_000, 10_000, 50_000, 100_000, 500_000, 1_000_000]
    x_axis_values = [1, 2, 3]
    for num_row in num_rows:
        total_time_fig, total_time_axis = plt.subplots(1, 1, figsize=(3, 2))
        pending_bars = []
        category_filtered = sorted(
            [
                (
                    res[0],
                    {
                        cat_res["query_num_join_count"]: cat_res
                        for cat_res in res[2]
                        if cat_res["query_num_num_rows"] == num_row
                    },
                )
                for res in all_results
            ],
            key=lambda x: PRESENT_CATEGORIES.index(x[0]),
        )
        x_axis = np.arange(len(x_axis_values))
        width = 0.2
        bar_width = width
        xticks = x_axis - (width / 2) + (len(category_filtered) * (width / 2))
        for category_idx, (category_key, category_result) in enumerate(
            category_filtered
        ):
            MARGIN = 0.0
            total_time_axis.margins(x=MARGIN)
            query_errors = list(
                [
                    (
                        (
                            q,
                            get_main_error(
                                category_result[q]["fail_reason"],
                                mock_data_options,
                            ),
                        )
                        if (q in category_result)
                        else (q, PlotErrors.not_available)
                    )
                    for q in x_axis_values
                ]
            )
            query_with_errors_mapped = get_query_error_mapping(query_errors)
            query_without_errors = query_with_errors_mapped.get(None, [])
            if query_without_errors == []:
                print("Mapping: ", query_with_errors_mapped)
            total_time_values = [
                (0 if q not in query_without_errors else category_result[q][time_key])
                for q in x_axis_values
            ]
            x_axis_shifted = x_axis + width * category_idx
            total_time_axis.bar(
                x_axis_shifted,
                total_time_values,
                width=bar_width,
                label=CATEGORY_LABEL[category_key],
                color=SYSTEM_COLORS[CATEGORY_LABEL[category_key]],
            )
            for error, error_queries in query_with_errors_mapped.items():
                if error is None:
                    continue
                error_queries_idx = list(map(x_axis_values.index, error_queries))
                bar_dict = dict(
                    x=[x_axis_shifted[idx] for idx in error_queries_idx],
                    height=0,
                    width=bar_width,
                    label=error,
                    color="lightgray",
                    edgecolor=SYSTEM_COLORS[CATEGORY_LABEL[category_key]],
                    hatch=ERROR_HATCHES[error],
                )
                pending_bars.append(bar_dict)

            xdata_yscale_t = blended_transform_factory(
                total_time_axis.transData, total_time_axis.transAxes
            )

            if True:
                total_time_axis.set_yscale("log")
            tp_add_grid_line(total_time_axis)

        for bar in pending_bars:
            bar["height"] = 0.85
            bar["transform"] = xdata_yscale_t
            total_time_axis.bar(**bar)

        total_time_axis.set_xticks(xticks, list(map(str, x_axis_values)))
        add_arrow_label(total_time_axis, "Total Time (s)", 0.0)
        total_time_axis.set_xlabel("Num. Joins")
        total_time_fig.savefig(
            out_dir / f"join_{backend_system}_{num_row}.pdf",
            bbox_inches="tight",
        )


def main():
    plot_context = BenchmarkPlot("plot_polynomial", ignore_args=True)
    plot_context.parser.add_argument("--db", required=True)
    parsed = plot_context.parser.parse_args()
    out_dir = plot_context.add_timestamp()

    db_name = Path(parsed.db).name.replace(".db", "")
    db_split = db_name.split("_")
    method = db_split[1]
    backend_system = db_split[2]
    conn = duckdb.connect(parsed.db)

    query = just_read("./overview_all_query.sql")
    if method == "factor":
        query_key = FACTOR_ID
        plot_func = plot_factor
    elif method == "join":
        query_key = JOIN_ID
        plot_func = plot_join
    else:
        assert False, f"Got method: {query_key}"
    query_key_joined = ",".join(query_key)
    query = query.replace("query_num", query_key_joined)
    list_query = make_list_query(query, "true")
    cursor = conn.cursor()
    results = run_query(cursor, list_query)
    print(results)
    plot_func(results, out_dir, backend_system, "total_usage_time")


if __name__ == "__main__":
    main()
