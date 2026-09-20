# Plots all the overview plots.
from functools import reduce
from itertools import product, zip_longest
from pathlib import Path
from statistics import mean, median
from typing import Tuple

from matplotlib import patches, pyplot as plt, ticker
import numpy as np

from traceprovpy.tools.benchmark_utils import get_strict_all_gprom_candidates
from traceprovpy.tools.extract_query_results import flatten
from traceprovpy.tools.file_utils import get_slowdown_cats, json_read_file, just_read
from utils import (
    ERROR_HATCHES,
    ERROR_PATCH_MAPPING,
    SYSTEM_COLORS,
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
    add_parallel_predicate,
    get_unique_handles_labels,
    make_list_query,
    query_categories,
    replace_extra_predicate,
    tp_add_grid_line,
)

import duckdb
from matplotlib.transforms import blended_transform_factory
from scipy.stats import gmean

CATEGORY_MAPPING = [
    ("base", SystemLabels.base),
    ("traceprov", SystemLabels.traceprov),
    ("gprom_window_heuristics", SystemLabels.gprom),
    ("smokedduck", SystemLabels.smokedduck),
    ("muller", SystemLabels.muller),
    ("provsql", SystemLabels.provsql),
]

PRESENT_CATEGORIES = [cat[0] for cat in CATEGORY_MAPPING]

CATEGORY_LABEL = {cat[0]: cat[1] for cat in CATEGORY_MAPPING}


def plot_legends(out_dir: Path):
    fig, ax = plt.subplots()
    handles_row_1 = []
    handles_row_2 = []
    for _, system_label in CATEGORY_MAPPING:
        patch = patches.Patch(color=SYSTEM_COLORS[system_label], label=system_label)
        handles_row_1.append(patch)
    for error_label, error_hatch in ERROR_PATCH_MAPPING:
        patch = patches.Patch(
            facecolor="lightgray",
            label=error_label,
            hatch=error_hatch,
            edgecolor="black",
        )
        handles_row_2.append(patch)
    handles = []
    # max_len = max(len())
    for top, bottom in zip_longest(handles_row_1, handles_row_2):
        if bottom is None:
            handles.append(top)
            continue
        handles.extend((top, bottom))
    # print(handles)
    generic_dump_legends(fig, ax, out_dir, handles, len(CATEGORY_MAPPING))


def make_slowdown_query(raw_query, time_key, extra_predicates: str):
    raw_query = replace_extra_predicate(raw_query, extra_predicates)
    return f"""
    with table_reduced as (
        select
            category,
            query_num,
            parallel,
            {time_key} as anchor_time,
            dense_rank() over (
                partition by parallel, query_num
                order by
                    {time_key}
            ) as dr
        from
            (
                {raw_query}
            )
        where
            fail_reason = ''
            and {time_key} is not NULL
            and category != 'base'
    ),
    table_traceprov_reduced as (
        select
            *
        from
            (
                select
                    *,
                    dense_rank() over (
                        partition by parallel, query_num
                        order by
                            dr
                    ) as dr_anchor
                from
                    (
                        select
                            *
                        from
                            table_reduced
                        where
                            category != 'traceprov'
                    )
            )
        where
            dr_anchor = 1
    )
    select
        (tr2.anchor_time / tr1.anchor_time) as slowdown,
        tr1.query_num as query_num,
        tr1.parallel as parallel
    from
        table_traceprov_reduced as tr1
        join table_reduced tr2 on tr1.query_num = tr2.query_num and tr1.parallel=tr2.parallel
    where
        tr2.category = 'traceprov';
    """


def get_all(parsed, _, extra_predicates: str, parallel=1):
    db_path = Path(parsed.db)
    connection = duckdb.connect(db_path)
    cursor = connection.cursor()
    sql = just_read("./overview_all_query.sql")
    cursor_safe_execute(cursor, make_list_query(sql, extra_predicates, parallel))
    results = cursor.fetchall()
    print("len results: ", len(results))
    cursor_safe_execute(
        cursor,
        make_slowdown_query(
            sql, ALL_TIME_KEY, add_parallel_predicate(extra_predicates, parallel)
        ),
    )
    slowdown_values = list(cursor.fetchall())
    # print(list(slowdown_values))
    cursor.close()
    # print("All Base Results: ", list([res for res in results if res[0] == "base"]))
    return results, slowdown_values


def cursor_safe_execute(cursor, query):
    # print("Running: ", query)
    cursor.execute(query)


def plot_offset(
    parsed, _, data_options: DataOptions, extra_predicates: str, parallel=1
):
    db_path = Path(parsed.db)
    data_options_all = data_options._replace(mode="all")
    db_all_path = db_path.parent / data_options_all.to_db()
    connection = duckdb.connect(db_all_path)
    cursor = connection.cursor()
    all_sql = just_read("./overview_all_query.sql")
    cursor_safe_execute(cursor, make_list_query(all_sql, extra_predicates, parallel))
    results_all = cursor.fetchall()
    results_all_base = [res for res in results_all if res[0] == "base"]
    cursor.close()
    connection.close()
    connection = duckdb.connect(db_path)
    cursor = connection.cursor()
    offset_sql = just_read("./overview_offset_query.sql")
    cursor_safe_execute(cursor, make_list_query(offset_sql, extra_predicates, parallel))
    results_offset = cursor.fetchall()
    base_added = [*results_offset, *results_all_base]
    cursor_safe_execute(
        cursor,
        make_slowdown_query(
            offset_sql,
            OFFSET_TIME_KEY,
            add_parallel_predicate(extra_predicates, parallel),
        ),
    )
    slowdown_values = list(cursor.fetchall())
    # print(list(slowdown_values))
    cursor.close()
    connection.close()
    # print(
    #     "Offset Base Results: ",
    #     list([res for res in results_all_base if res[0] == "base"]),
    # )
    return base_added, slowdown_values


def _formatter(val, _):
    if val == 1.0:
        return str(1)
    if val == 10.0:
        return str(10)
    return str(val)


def get_width(category_length):
    if category_length == 4:
        return 0.2, 0.2, 1
    if category_length == 5:
        return 0.2, 0.2, 1.15
    assert False, f"got {category_length}"


def plot_main(
    results: list[Tuple[str, dict]],
    options: DataOptions,
    out_dir: Path,
    time_key: str,
    raw_slowdown: list[float],
):
    results_filtered = [result for result in results if result[0] in PRESENT_CATEGORIES]
    results_sorted = list(
        sorted(results_filtered, key=lambda x: PRESENT_CATEGORIES.index(x[0]))
    )
    x_axis_categories = query_categories()
    x_axis_combined = [cat.queries for cat in x_axis_categories]
    x_axis_values = [str(_query) for _queries in x_axis_combined for _query in _queries]
    category_count = len(results_sorted)
    width, bar_width, group_gap = get_width(category_count)
    x_axis = np.arange(len(x_axis_values)) * group_gap
    total_time_fig, (total_time_axis, slowdown_box_plot_axis) = plt.subplots(
        1,
        2,
        figsize=(13, 2),
        gridspec_kw={"hspace": 0.5, "wspace": 0.1, "width_ratios": [19, 1]},
    )

    # bar_width = 0.2
    MARGIN = 0.0
    total_time_axis.margins(x=MARGIN)
    slowdown_box_plot_axis.margins(x=MARGIN)
    pending_bars = []
    query_time_map = dict()
    query_error_map = dict()
    for category_idx, (category_key, category_data) in enumerate(results_sorted):
        category_data_mapped = make_category_data_mapped(category_data)
        query_errors = list(
            [
                (
                    (q, get_main_error(category_data_mapped[q]["fail_reason"], options))
                    if (q in category_data_mapped)
                    else (q, PlotErrors.not_available)
                )
                for q in x_axis_values
            ]
        )
        query_with_errors_mapped = get_query_error_mapping(query_errors)
        query_without_errors = query_with_errors_mapped[None]
        total_time_values = [
            (0 if q not in query_without_errors else category_data_mapped[q][time_key])
            for q in x_axis_values
        ]
        print(category_key, total_time_values, query_without_errors)
        absolute_time_value_map = {
            q: (
                None
                if q not in query_without_errors
                else category_data_mapped[q][time_key]
            )
            for q in x_axis_values
        }
        query_time_map[category_key] = absolute_time_value_map
        query_error_map[category_key] = query_errors
        x_axis_shifted = x_axis + width * category_idx
        total_time_axis.bar(
            x_axis_shifted,
            total_time_values,
            width=bar_width,
            label=CATEGORY_LABEL[category_key],
            color=SYSTEM_COLORS[CATEGORY_LABEL[category_key]],
        )
        total_time_axis.set_yscale("log")
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
    for bar in pending_bars:
        bar["height"] = 0.85
        bar["transform"] = xdata_yscale_t
        total_time_axis.bar(**bar)
    legend_labels_all, handles_all = get_unique_handles_labels(total_time_axis)

    # leg = total_time_axis.legend(
    #     handles=handles_all,
    #     labels=legend_labels_all,
    #     loc="center right",
    #     bbox_to_anchor=(1.10, 0.95),
    #     prop=dict(size=9),
    # )
    # for handle, label in zip(leg.legend_handles, legend_labels_all):
    #     if label in ERROR_HATCHES:
    #         handle.set_edgecolor("black")
    total_time_axis.set_xticks(
        x_axis - width / 2 + (len(results_sorted) * (width / 2)), x_axis_values
    )
    colors = ["#d6d6d6", "white"]
    running = 0
    last_end = 0
    for _idx, query_category in enumerate(x_axis_combined):
        old_running = running
        running += len(query_category)
        span_end = x_axis[-1] + ((category_count) - 1) * width
        if _idx < len(x_axis_combined) - 1:
            _prev_start = x_axis[(running - 1)] + (category_count - 1) * width
            _prev_end = x_axis[(running)]
            norm_middle = (_prev_start + _prev_end) / 2
            span_end = norm_middle
            total_time_axis.vlines(
                x=norm_middle,
                colors="black",
                linestyles="solid",
                ymin=0,
                ymax=1,
                linewidth=1,
                transform=xdata_yscale_t,
            )
        new_running = running
        middle = int((old_running + new_running) / 2)
        true_label = x_axis_categories[_idx].label
        total_time_axis.text(
            middle * group_gap,
            1.02,
            true_label,
            transform=xdata_yscale_t,
            ha="center",
            va="bottom",
            fontsize=9,
        )
        start_ = last_end
        if _idx == 0:
            start_ -= width
        total_time_axis.axvspan(
            start_,
            span_end,
            color=colors[_idx % 2],
            zorder=0,
        )
        last_end = span_end
    tp_add_grid_line(total_time_axis)
    total_time_axis.yaxis.set_major_formatter(ticker.FuncFormatter(_formatter))
    add_arrow_label(total_time_axis, "Total Time (s)", 0.0)
    # total_time_axis.set_ylim(bottom=0.0)
    slowdown_sorted = sorted(raw_slowdown, key=lambda x: x[0])
    slowdown_map = {x[1]: x[0] for x in slowdown_sorted}
    slowdown_data = [x[0] for x in slowdown_sorted]
    print("Using slowdown data: ", slowdown_data)
    slowdown_box_plot_axis.boxplot(slowdown_data, whis=(0, 100), showfliers=False)
    # slowdown_box_plot_axis.axhline(y=1, color='red', linestyle='--', linewidth=2)
    # slowdown_box_plot_axis.axhline(y=2, color='red', linestyle='--', linewidth=2)
    tp_add_grid_line(slowdown_box_plot_axis)
    slowdown_soft_limit = np.percentile(slowdown_data, 75)
    max_slowdown = max(slowdown_data)
    slowdown_box_plot_axis.set_ylim(top=min(slowdown_soft_limit * 1.6, 5), bottom=0)
    slowdown_box_plot_axis.set_xticks([])
    slowdown_box_plot_axis.minorticks_on()
    frac = 0.15
    slowdown_box_plot_axis.set_xlim(1 - frac, 1 + frac)
    sd_xdata_yscale_t = blended_transform_factory(
        slowdown_box_plot_axis.transData, slowdown_box_plot_axis.transAxes
    )
    max_sd_trunc = round(max_slowdown, 1)
    avg = round(gmean(slowdown_data), 1)
    slowdown_box_plot_axis.text(
        1,
        1.02,
        f"Max: {max_sd_trunc}x",
        transform=sd_xdata_yscale_t,
        ha="center",
        va="bottom",
        fontsize=9,
    )
    slowdown_box_plot_axis.text(
        1,
        -0.1,
        f"G. Mean: {avg}x",
        transform=sd_xdata_yscale_t,
        ha="center",
        va="top",
        fontsize=9,
    )

    print("slowdown map", slowdown_map)
    query_cat_slowdown = [
        (query_cat_idx, slowdown_map[query])
        for query_cat_idx, query_cat in enumerate(x_axis_categories)
        for query in list(map(str, query_cat.queries))
        if query in slowdown_map
    ]
    slowdown_quantiles = list(np.percentile(slowdown_data, [25, 50, 75]))
    print("slowdown_quantiles", slowdown_quantiles)
    slowdown_cat_percentiles = get_slowdown_cats(query_cat_slowdown, slowdown_quantiles)
    print(
        "slowdown_cat_percentiles",
        slowdown_cat_percentiles,
        len(raw_slowdown),
        len(flatten(slowdown_cat_percentiles)),
    )
    slowdown_top = list(map(get_category_by_top, slowdown_cat_percentiles))
    print("slowdown_top", slowdown_top)
    total_time_fig.savefig(
        out_dir / "overview_total_time.pdf",
        bbox_inches="tight",
    )
    plot_legends(out_dir)
    return query_time_map, query_error_map


ALL_TIME_KEY = "total_usage_time"
OFFSET_TIME_KEY = "total_usage_time"


def get_category_by_top(elems):
    count_dict = dict()
    for elem in elems:
        count_dict = {**count_dict, elem: count_dict.get(elem, 0) + 1}
    return sorted(count_dict.items(), key=lambda tup: tup[1], reverse=True)


def get_extra_predicates(parsed, data_options: DataOptions):
    db_name = data_options.get_db_name()
    attr = f"{db_name}_tpch_dir"
    tpch_dir = Path(getattr(parsed, attr))
    true_sf = data_options.true_sf()
    if data_options.mode == "all":
        gprom_dir = "gprom_params_default_restricted"
    else:
        gprom_dir = "gprom_params_default_restricted_keys"
    if db_name == "duckdb":
        gprom_dir = f"{gprom_dir}_optimized"
    gprom_path = tpch_dir / f"scale_{true_sf}" / gprom_dir / "config_gprom.json"
    assert gprom_path.exists(), f"expected {gprom_path} to exist"
    config_gprom = json_read_file(gprom_path)
    config_gprom_map = {
        int(q): qdata for (q, qdata) in config_gprom.items() if q not in ["call_mode"]
    }
    all_gprom_condidates = get_strict_all_gprom_candidates()
    absent = []
    for query in range(1, 23):
        current_gprom = config_gprom_map.get(query, dict())
        for gprom_candidate in all_gprom_condidates:
            query_gprom_value = current_gprom.get(gprom_candidate.to_str(), dict())
            passed = query_gprom_value.get("passed", False)
            if not passed:
                absent.append((str(query), gprom_candidate.safe_key()))
    print("ABSENT", absent)
    absent_predicates = [
        f"not (query_num='{query_num}' and category='{gprom_category}')"
        for (query_num, gprom_category) in absent
    ]
    all_preds = " and ".join(absent_predicates)
    return all_preds


def run_and_get_time_map(plot_context, parsed, sf, strict=True):
    modes = ["all", "offset"]
    dbs = ["postgres", "duckdb"]
    result_map = dict()
    for mode, db in product(modes, dbs):
        db_option = DataOptions(sf=f"sf_{sf}", db_name=db, mode=mode)
        parsed.db = Path(parsed.db_dir) / db_option.to_db()
        assert not strict or parsed.db.exists(), f"Expected {parsed.db} to exist!"
        if not parsed.db.exists():
            continue
        if hasattr(parsed, "_true_out_dir"):
            delattr(parsed, "_true_out_dir")
        out_dir = plot_context.add_timestamp()
        data_options = get_options_split(parsed)
        print(data_options)
        time_key = None
        extra_predicates = get_extra_predicates(parsed, data_options)
        if data_options.mode == "all":
            results, slowdown = get_all(parsed, out_dir, extra_predicates)
            time_key = ALL_TIME_KEY
        elif data_options.mode == "offset":
            results, slowdown = plot_offset(
                parsed, out_dir, data_options, extra_predicates
            )
            time_key = OFFSET_TIME_KEY
        result_map[db_option] = plot_main(
            results, data_options, out_dir, time_key, slowdown
        )
        # print("Result layer base: ", result_map[db_option][0]["base"])
    return result_map


def make_overview_base_parser(plot_context: BenchmarkPlot):
    plot_context.parser.add_argument("--db_dir", required=True)
    plot_context.parser.add_argument(
        "--postgres_tpch_dir", required=False, default=("../postgres/benchmark/tpch/")
    )
    plot_context.parser.add_argument(
        "--duckdb_tpch_dir", required=False, default=("../duckdb/benchmark/tpch/")
    )


def main():
    plot_context = BenchmarkPlot("plot_overview", ignore_args=True)
    make_overview_base_parser(plot_context)
    plot_context.parser.add_argument("--sf", required=True, type=int)
    parsed = plot_context.parser.parse_args()
    run_and_get_time_map(plot_context, parsed, parsed.sf)


if __name__ == "__main__":
    main()
