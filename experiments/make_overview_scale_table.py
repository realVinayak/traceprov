from functools import reduce
from pathlib import Path
from typing import List, NamedTuple, Tuple

from matplotlib import pyplot as plt
import numpy as np

old_settings = np.seterr(all="raise")

from traceprovpy.tools.file_utils import just_write
from utils import SYSTEM_COLORS, SYSTEM_MARKERS, DataOptions, PlotErrors
from plot_overview import (
    CATEGORY_LABEL,
    make_overview_base_parser,
    run_and_get_time_map,
    PRESENT_CATEGORIES,
)
from traceprovpy.tools.plot_utils import (
    BenchmarkPlot,
    time_func_formatter,
    tp_add_grid_line,
)

SF = ["01", "10", "100"]


def resolve_value(raw_value):
    if raw_value is None:
        return "-"
    # if raw_value > 100 or raw_value < 0.01:
    #     # give up.
    #     return f"{raw_value:.2e}"
    if raw_value >= 1_000:
        return f"{round(raw_value / 1_000, 1)} K"
    if raw_value > 1:
        return str(round(raw_value, 1))
    if raw_value <= 1 and raw_value > 0:
        if raw_value >= 0.1:
            return str(round(raw_value, 2))
        else:
            return str(round(raw_value, 3))
        # return f"{raw_value:.2e}"
    return str(round(raw_value, 2))


class Cell(NamedTuple):
    data_option: DataOptions
    scale_category_results: List[Tuple[int, List[Tuple[str, List[float]]]]]
    query_axis: List[str]

    def to_latex(self):
        total_width = len(self.query_axis) + 2  # +1 for the SF label +1 for category
        nice_name = f"{self.data_option.get_db_name()}_{self.data_option.mode}.tex"
        all_cs = "|".join(["c"] * (total_width))
        replacer = [
            ("COLS", str(total_width)),
            (
                "DB_MODE",
                f"{self.data_option.get_db_name().capitalize()} - {self.data_option.mode}",
            ),
            ("COLDEF", f"|{all_cs}|"),
        ]

        def _reducer(prev, curr):
            key, value = curr
            return prev.replace(key, value)

        header = [
            "SF",
            "Category",
            *["\\tpchquery{Q}".replace("Q", q) for q in self.query_axis],
        ]

        def apply_reduce(val):
            return reduce(_reducer, replacer, val)

        START_LINES = [
            apply_reduce("\\begin{tabular}{COLDEF}"),
            "\\hline",
            apply_reduce("\\multicolumn{COLS}{c}{DB_MODE} \\\\"),
            "\\hline",
        ]
        END_LINES = ["\\hline", "\\end{tabular}"]
        lines = []
        CMIDRULE = apply_reduce("\\cline{2-COLS}")
        for scale, scale_results in self.scale_category_results:
            cat_length = len(scale_results)
            for cat_idx, (category, category_result) in enumerate(scale_results):
                cells = []
                if cat_idx == 0:
                    cells.append(
                        "\\multirow{CATLEN}{*}{SCALE}".replace(
                            "CATLEN", str(cat_length)
                        ).replace("SCALE", str(int(scale)))
                    )
                    lines.append(["\\hline"])
                else:
                    cells.append("")
                    lines.append([CMIDRULE])
                cells.append(CATEGORY_LABEL[category])
                for qresult in category_result:
                    cells.append(resolve_value(qresult))
                lines.append(cells)
        all_lines = [header, *lines]
        all_lines_joined = " \n ".join(
            [
                (
                    " & ".join(cell)
                    + (" \\\\ " if "hline" not in cell and "cline" not in cell else "")
                )
                for cell in all_lines
            ]
        )
        all_lines_with_block = [*START_LINES, all_lines_joined, *END_LINES]
        return nice_name, "\n".join(all_lines_with_block)


def main():
    plot_context = BenchmarkPlot("overview_scale_table", ignore_args=True)
    make_overview_base_parser(plot_context)
    parsed = plot_context.parser.parse_args()
    all_scale_results = dict()
    for scale_factor in SF:
        scale_result = run_and_get_time_map(plot_context, parsed, scale_factor, False)
        just_write("./tmp/all_scale_results.py", str(all_scale_results))
        for data_option, results in scale_result.items():
            norm_data_option = data_option.normalize_sf()
            current = all_scale_results.get(norm_data_option, {})
            assert scale_factor not in current
            all_scale_results[norm_data_option] = {
                **all_scale_results.get(norm_data_option, {}),
                scale_factor: results,
            }
    print("ALL SCALE RESULTS", all_scale_results)

    query_axis = list(map(str, range(1, 23)))
    all_cells = []
    for data_option, data_option_results in all_scale_results.items():
        scale_category_results = []
        for scale, combined_scale_results in sorted(
            data_option_results.items(), key=lambda x: int(x[0])
        ):
            scale_results, _ = combined_scale_results
            baseline = scale_results["base"]
            category_results_flat = []
            for category, category_results in sorted(
                scale_results.items(), key=lambda x: PRESENT_CATEGORIES.index(x[0])
            ):
                if category == "base":
                    continue
                category_slowdown = [
                    (
                        None
                        if category_results[query] is None
                        else category_results[query] / baseline[query]
                    )
                    for query in query_axis
                ]
                category_results_flat.append((category, category_slowdown))
            scale_category_results.append((scale, category_results_flat))
        current_cell = Cell(
            data_option=data_option,
            scale_category_results=scale_category_results,
            query_axis=query_axis,
        )
        all_cells.append(current_cell)

    for cell in all_cells:
        file, contents = cell.to_latex()
        just_write(parsed._true_out_dir / file, contents)
    plot_total_time_plots(all_scale_results, parsed._true_out_dir)


IGNORE_SYSTEMS = ["provsql"]


def plot_total_time_plots(all_scale_results: dict, outdir: Path):
    for data_option, data_option_results in all_scale_results.items():
        print("Handling", data_option)
        all_queries = set(list(map(str, range(1, 23))))
        # MARGIN = 0.02
        # total_time_axis.margins(x=MARGIN)
        scale_values = []
        true_scale_values = []

        # ALl queries are considered valid now.
        for scale, combined_scale_results in sorted(
            data_option_results.items(), key=lambda x: int(x[0])
        ):
            scale_results, error_list = combined_scale_results
            # scale_values.append(f"S={scale}")
            # true_scale_values.append(scale)
            for category, category_results in scale_results.items():
                if category in IGNORE_SYSTEMS:
                    continue
                valid_queries = set(
                    [q for q, val in category_results.items() if val is not None]
                )
                print(valid_queries)
                all_queries = all_queries.intersection(valid_queries)

        category_result_map = {}
        for scale, combined_scale_results in sorted(
            data_option_results.items(), key=lambda x: int(x[0])
        ):
            scale_results, error_list = combined_scale_results
            scale_values.append(f"S={int(scale)}")
            true_scale_values.append(scale)
            for category, category_results in scale_results.items():
                query_error_mapping = {q: q_err for (q, q_err) in error_list[category]}
                if category in IGNORE_SYSTEMS:
                    continue
                total_category_time = 0.0
                for query in all_queries:
                    # need to handle unknown results (if they are null.)
                    current_time = category_results[query]
                    assert current_time != 0, "Got time as 0!!"
                    if current_time is not None:
                        total_category_time += current_time
                        continue
                    assert False
                    assert (
                        query in query_error_mapping
                    ), f"Expected {query} to be found in {query_error_mapping}"
                    query_error = query_error_mapping[query]
                    if query_error == PlotErrors.timeout:
                        total_category_time += data_option.get_timeout_value()
                        print("Handling error via timeout!: ", scale, category, query)
                        continue
                    baseline_result = combined_scale_results[0]["base"][query]
                    total_category_time += baseline_result
                    # if category == 'smokedduck':
                    #    print("Handling SmokedDuck invalid case!")
                if category not in category_result_map:
                    category_result_map[category] = dict()
                category_result_map[category][scale] = total_category_time

        scale_factor_axis = np.arange(len(data_option_results))
        total_time_fig, total_time_axis = plt.subplots(
            1, 1, figsize=(len(scale_factor_axis) / 1.5, 2)
        )
        total_time_axis.margins(x=0.2)
        # scale_values = [f"SF={sf}" for sf in SF]
        # all_queries = set(list(map(str, range(1, 23))))
        for category_key, category_results in sorted(
            category_result_map.items(), key=lambda x: PRESENT_CATEGORIES.index(x[0])
        ):
            scale_adjusted = [
                (category_results[sv] if sv in category_results else None)
                for sv in true_scale_values
            ]
            true_cat_label = CATEGORY_LABEL[category_key]
            total_time_axis.plot(
                scale_factor_axis,
                scale_adjusted,
                color=SYSTEM_COLORS[true_cat_label],
                marker=SYSTEM_MARKERS[true_cat_label],
                markerfacecolor="none",
                markeredgecolor=SYSTEM_COLORS[true_cat_label],
                # linestyle="-",
            )

        total_time_axis.set_yscale("log")
        total_time_axis.set_xticks(scale_factor_axis, scale_values)
        total_time_axis.yaxis.set_major_formatter(time_func_formatter())
        tp_add_grid_line(total_time_axis)
        # total_time_axis.legend()
        total_time_fig.savefig(
            outdir / f"{data_option.normalize_sf().to_db()}_total_time_scale.pdf",
            bbox_inches="tight",
        )


if __name__ == "__main__":
    main()
