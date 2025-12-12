from functools import reduce
import glob
import json
from traceprovpy.tools.plot_utils import BenchmarkPlot, Plotable
import matplotlib.pyplot as plt


# POST()
# 0.01, 0.05, 1.0, 5.0, 10, 50, 75
# ---- 1M, 5M ----


def assert_len_one(array):
    assert len(array) == 1
    if isinstance(array[0], list):
        return assert_len_one(array[0])
    else:
        return array[0]


class DuckDbBenchmarkPlot(BenchmarkPlot):
    ...
    # def assert_values_same(self, in_dict: dict):
    #     reduced_set = set()
    #     for key, values in in_dict.items():
    #         assert len(set(values)) <= 1
    #         if len(set(values)) == 0:
    #             continue
    #         reduced_set.add(str(values[0]))
    #         print("key:", key, "unique value:", list(set(values)))
    #     assert len(reduced_set) <= 1, f"{len(reduced_set)}, {reduced_set}"

    # def sanity_checks(self, result):
    #     result_spec: dict = result["result"]
    #     for scale, scale_result in result_spec.items():
    #         for mode, mode_result in scale_result.items():
    #             for method, method_result in mode_result.items():
    #                 if "traceprov" in method:
    #                     self.assert_values_same()
    #                     extras = method_result["extras"]
    #                     for extra in extras:
    #                         extra_count = extra["traceprov_infer_count"]


def merge_query(
    query_result_first: dict[str, dict], query_result_second: dict[str, dict]
):
    # These two should never have anything in common.
    print(set(query_result_first.keys()), set(query_result_second.keys()))
    assert set(query_result_first.keys()).isdisjoint(set(query_result_second.keys()))
    merged_result = {**query_result_first, **query_result_second}
    assert len(merged_result) == (len(query_result_first) + len(query_result_second))
    return merged_result


def merge_directory(first: dict, second: dict):
    return {
        **first,
        **{
            query_key: (
                merge_query(first[query_key], query) if query_key in first else query
            )
            for query_key, query in second.items()
        },
    }


def merge_results(first: dict, second: dict):
    return {
        **first,
        **{
            directory_key: (
                merge_directory(first[directory_key], directory)
                if directory_key in first
                else directory
            )
            for (directory_key, directory) in second.items()
        },
    }


def resolve_result(method, method_result):
    selectivity = float(method.split("_")[-1])
    plotables: list[Plotable] = []
    if "traceprov" in method:
        plotables.append(
            Plotable(
                label="T_infer",
                values=[
                    assert_len_one(extra["traceprov_infer_time"]["captured"]) / 1000
                    for extra in method_result["extras"]
                ],
            )
        )
        plotables.append(
            Plotable(
                label="T_mat",
                values=[
                    extra["traceprov_materialize"]["explain_time"]
                    for extra in method_result["extras"]
                ],
            )
        )
        plotables.append(
            Plotable(
                label="T_count",
                values=[
                    assert_len_one(extra["traceprov_infer_count"]["captured"])
                    for extra in method_result["extras"]
                ],
            )
        )
    else:
        assert "DUCKDB" in method
        plotables.append(
            Plotable(
                label="D_temp_table",
                values=[
                    result["result_mode_temp_table"]["time"] for result in method_result
                ],
            )
        )
        plotables.append(
            Plotable(
                label="D_table",
                values=[
                    result["result_mode_table"]["time"] for result in method_result
                ],
            )
        )
        plotables.append(
            Plotable(
                label="D_select",
                values=[
                    result["result_mode_select"]["time"] for result in method_result
                ],
            )
        )
        plotables.append(
            Plotable(
                label="D_cursor",
                values=[
                    result["result_mode_cpp"]["time"] / 1000 for result in method_result
                ],
            )
        )
    return_dict = {
        (float(selectivity), plotable.label): plotable.compute_median()
        for plotable in plotables
    }
    assert len(return_dict) == len(plotables)
    return return_dict


def _reduce(accum, current):
    key_pair = current[0]
    label = key_pair[1]
    return {**accum, label: [*accum.get(label, []), (key_pair[0], current[1])]}


import numpy as np


def plot_results(combined_results: dict):
    # columns = ["T_infer", "T_mat", "D_temp_table", "D_select", "D_cursor"]
    columns = ["T_infer", "D_temp_table", "D_select", "D_cursor"]
    for scale, scale_result in combined_results.items():
        scale_result_mapped = reduce(
            lambda x, y: {**x, **resolve_result(y[0], y[1])},
            scale_result["predicate_post"].items(),
            {},
        )
        specific_results = {
            key: value
            for key, value in scale_result_mapped.items()
            if key[1] in columns
        }
        remapped = reduce(_reduce, specific_results.items(), {})
        remapped_sorted = {
            key: sorted(values, key=lambda x: x[0])
            for (key, values) in remapped.items()
        }
        print(remapped_sorted)
        x_axis = [tuple(v[0] for v in values) for values in remapped_sorted.values()]
        assert len(set(x_axis)) == 1
        x_axis_selected = x_axis[0]
        print(x_axis_selected)
        plt.clf()
        figure, axis = plt.subplots()
        width = 0.2
        x_axis_uniform = np.arange(len(x_axis_selected))
        for cat_id, (category, category_data) in enumerate(
            sorted(remapped_sorted.items(), key=lambda x: columns.index(x[0]))
        ):
            offset = cat_id * width
            bars = axis.bar(
                x_axis_uniform + offset,
                [cat[1] for cat in category_data],
                width,
                label=category,
            )
            axis.bar_label(bars, fmt="%.2f", fontsize=6, rotation=60, padding=3)

        axis.set_title(f"Inference time comparison: {scale} rows")
        axis.set_xticks(x_axis_uniform + width, x_axis_selected)
        axis.set(xlabel="Selectivity (%)", ylabel="Execution time (s)")
        axis.legend()
        plt.savefig(f"{scale}.png")


def reduce_results(combined_result: dict):
    result = {
        scale: sorted(
            reduce(
                lambda x, y: {**x, **resolve_result(y[0], y[1])},
                scale_result["predicate_post"].items(),
                {},
            ).items(),
            key=lambda x: x[0],
        )
        for (scale, scale_result) in combined_result.items()
    }

    headers = [
        tuple(f"{v[0][0]}_{v[0][1]}" for v in value) for value in result.values()
    ]
    assert len(set(headers)) == 1
    new_header = ("scale", *headers[0])
    table = [
        (scale, *tuple(str(v[1]) for v in contents))
        for (scale, contents) in result.items()
    ]
    # print(table)
    len_table = set([len(row) for row in table])
    assert len(len_table) == 1
    assert len(table[0]) == len(
        new_header
    ), f"mismatch: {len(len_table)}, {len(new_header)}"
    all_rows = [new_header, *table]
    return all_rows


def main():
    analyze_plotter = DuckDbBenchmarkPlot("duckdb_selectivity_plotter")
    parsed = analyze_plotter.parser.parse_args()
    all_results = []
    for file in parsed.files:
        complete_path = f"{parsed.root}/{file}/main_result.json"
        paths = glob.glob(complete_path)
        for path in paths:
            with open(path) as f:
                main_result = json.load(f)
            all_results.append(main_result["result"])
            print("checking: ", path)
    combined_results = reduce(merge_results, all_results)
    rows = reduce_results(combined_results)
    with open("combined.json", "w") as f:
        f.write(json.dumps(combined_results, indent=4))
    tsv = "\n".join(["\t".join(row) for row in rows])
    with open("duckdb_analyzed.tsv", "w") as f:
        f.write(tsv)
    plot_results(combined_results)


if __name__ == "__main__":
    main()
