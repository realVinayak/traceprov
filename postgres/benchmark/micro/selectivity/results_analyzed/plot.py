from functools import reduce
from typing import Any
from traceprovpy.tools.plot_utils import BenchmarkPlot, Plotable
import json
import glob
import matplotlib.pyplot as plt


def assert_len_one(array):
    assert len(array) == 1
    if isinstance(array[0], list):
        return assert_len_one(array[0])
    else:
        return array[0]


strip_last_under = lambda in_str: "_".join(in_str.split("_")[:-1])


class SelectivityBenchmarkPlot(BenchmarkPlot):
    def do_check_on_query(self, query_result: dict[str, dict]):
        gprom_count = dict()
        traceprov_count = dict()
        for query_key, query_result in query_result.items():
            if query_key.startswith("gprom"):
                query_key_adjusted = strip_last_under(query_key)
                gprom_query_count = [
                    assert_len_one(extra["count"]["captured"])
                    for extra in query_result["extras"]
                ]
                assert query_key_adjusted not in gprom_count
                gprom_count[query_key_adjusted] = gprom_query_count
            elif query_key.startswith("traceprov"):
                query_key_adjusted = strip_last_under(query_key)
                traceprov_query_count = [
                    assert_len_one(extra["traceprov_infer_count"]["captured"])
                    for extra in query_result["extras"]
                ]
                assert query_key_adjusted not in traceprov_count
                traceprov_count[query_key_adjusted] = traceprov_query_count

        print(gprom_count)
        print(traceprov_count)
        self.assert_values_same({**gprom_count, **traceprov_count})
        # self.assert_values_same(traceprov_count)

    def assert_values_same(self, in_dict: dict):
        reduced_set = set()
        for key, values in in_dict.items():
            assert len(set(values)) <= 1
            if len(set(values)) == 0:
                continue
            reduced_set.add(str(values[0]))
            print("key:", key, "unique value:", list(set(values)))
        assert len(reduced_set) <= 1, f"{len(reduced_set)}, {reduced_set}"

    def sanity_checks(self, main_result):
        # For a given result, the infer from gprom and traceprov should be equal.
        # assert just that.
        result = main_result["result"]
        for directory in result:
            for query in result[directory]:
                self.do_check_on_query(result[directory][query])

    def plot(self, **kwargs):
        super().plot(**kwargs)


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


PLOT_ORDER_SELECTIVITY_BENCH = [
    "base",
    "gprom_join",
    "gprom_join (materialize)",
    "gprom_join_heuristics",
    "gprom_join_heuristics (materialize)",
    "gprom_window",
    "gprom_window (materialize)",
    "gprom_window_heuristics",
    "gprom_window_heuristics (materialize)",
    "traceprov (F)",
    "traceprov (F + S + I)",
    "traceprov (F + S + I + M)",
]

PLOT_ORDER_RESTRICTED_SELECTIVITY_BENCH = [
    "base",
    "gprom_join (materialize)",
    "gprom_join_heuristics (materialize)",
    "gprom_window (materialize)",
    "gprom_window_heuristics (materialize)",
    # "traceprov (F)",
    # "traceprov (F + S + I)",
    "traceprov (F + S + I + M)",
]

PLOT_ORDER_TRACEPROV = [
    "base",
    "traceprov (F)",
    "traceprov (F + S + I)",
    "traceprov (F + S + I + M)",
]


def get_explain_time(result_list: list[dict[str, float]]):
    explain_times = []
    for result in result_list:
        if "explain_time" not in result:
            assert "timeout" in result
            print(result)
            return []
        explain_times.append(result["explain_time"])
    return explain_times


def merge_plotables(
    first: dict[str, list[Plotable]], second: dict[str, list[Plotable]]
) -> dict[str, list[Plotable]]:
    return {
        **first,
        **{key: [*first.get(key, []), *value] for key, value in second.items()},
    }


def _get_plotable_from_query_result(
    key: str, result: dict
) -> dict[str, list[Plotable]]:
    group = key.split("_")[-1]
    # we don't need the float value, but this is a nice sanity check.
    float(group)

    def return_in_dict(*plotable: Plotable):
        assert all(
            [_plotable.label in PLOT_ORDER_SELECTIVITY_BENCH for _plotable in plotable]
        )
        return {group: plotable}

    if "base" in key:
        return return_in_dict(
            Plotable(label="base", values=get_explain_time(result["base"]))
        )

    if "gprom" in key:
        # Go from most specific to least specific.
        simple_key = strip_last_under(key.replace("_selectivity", ""))
        return return_in_dict(
            Plotable(
                label=f"{simple_key}",
                values=get_explain_time(result["base"]),
            ),
            Plotable(
                label=f"{simple_key} (materialize)",
                values=get_explain_time(result["materialize"]),
            ),
        )

    if "traceprov" in key:
        forward_times = get_explain_time(result["base"])
        infer_times = [
            assert_len_one(extra["traceprov_infer_time"]["captured"]) / 1000
            for extra in result["extras"]
        ]
        sync_times = [
            assert_len_one(extra["traceprov_sync_time"]["captured"]) / 1000
            for extra in result["extras"]
        ]

        assert len(forward_times) == len(infer_times)
        assert len(forward_times) == len(sync_times)
        materialization_time = [
            extra["traceprov_materialize"]["explain_time"] for extra in result["extras"]
        ]

        f_s_i_summed = [
            f + s + i for f, i, s in zip(forward_times, infer_times, sync_times)
        ]
        assert len(f_s_i_summed) == len(forward_times)

        f_s_i_m_summed = [
            f + s + m
            for (f, s, m) in zip(forward_times, sync_times, materialization_time)
        ]
        assert len(f_s_i_m_summed) == len(forward_times)

        forward = Plotable(label="traceprov (F)", values=forward_times)
        forward_sync_infer = Plotable(
            label="traceprov (F + S + I)", values=(f_s_i_summed)
        )

        forward_sync_infer_materialize = Plotable(
            label="traceprov (F + S + I + M)", values=f_s_i_m_summed
        )

        return return_in_dict(
            forward, forward_sync_infer, forward_sync_infer_materialize
        )

    raise Exception(f"Shouldn't reach here: {key}, {result}")


EXTRACTED_PLOTABLES = dict[str, dict[str, dict[str, list[Plotable]]]]


def get_plotables(combined_result: dict[str, dict[str, dict[str, Any]]]):
    # each dir will have its own plotable (for now...)
    return {
        dir_name: {
            query_name: {
                category: sorted(
                    cat_result,
                    key=lambda x: PLOT_ORDER_SELECTIVITY_BENCH.index(x.label),
                )
                for (category, cat_result) in reduce(
                    merge_plotables,
                    [
                        _get_plotable_from_query_result(key, result)
                        for (key, result) in query_results.items()
                    ],
                ).items()
            }
            for query_name, query_results in dir_results.items()
        }
        for (dir_name, dir_results) in combined_result.items()
    }


QUERY_RESULT_ORDER = ["predicate_post", "predicate_pre"]

import numpy as np

WIDTH = 0.06


def plot_plotables(
    extracted: EXTRACTED_PLOTABLES,
    plot_order,
    suffix="",
    use_log=True,
    add_stddev=False,
    add_values=False,
    width=WIDTH,
    bbox_to_anchor=(1, 0.7),
):
    for directory, queries_per_dir in extracted.items():
        plt.clf()
        figure, axis = plt.subplots(
            1, len(queries_per_dir), sharey=True, layout="constrained"
        )
        slowdown_figure, slowdown_axis = plt.subplots(
            1, len(queries_per_dir), sharey=True, layout="constrained"
        )

        print("AXIS", axis)
        print("DIR: ", directory)
        figure.set_size_inches(18, 7)
        slowdown_figure.set_size_inches(18, 7)

        figure.suptitle(f"Execution Time vs Selectivity ({directory})")
        slowdown_figure.suptitle("Slowdown vs Selectivity")
        sorted_queries_per_dir = sorted(
            queries_per_dir.items(),
            key=lambda q_name: QUERY_RESULT_ORDER.index(q_name[0]),
        )
        for _idx, (query, query_result) in enumerate(sorted_queries_per_dir):
            x_axis = np.arange(len(query_result))
            category_sorted = sorted(query_result.items(), key=lambda qr: float(qr[0]))
            basetime = dict()
            for cat_id, expected_category in enumerate(plot_order):
                offset = cat_id * width
                # First, try to find this one.
                founds = []
                stddevs = []
                slowdowns = []

                for category_name, category_values in category_sorted:
                    print(query, category_name)
                    _finds = [
                        c for c in category_values if c.label == expected_category
                    ]
                    if len(_finds) == 0:
                        found = 0
                        stddev = 0
                    else:
                        assert len(_finds) == 1
                        found = _finds[0].compute_median()
                        stddev = _finds[0].compute_stddev()
                    if cat_id == 0:
                        basetime[category_name] = found
                    else:
                        slowdowns.append(found / basetime[category_name])
                    founds.append(found)
                    stddevs.append(stddev)
                if add_stddev:
                    stdev_args = dict(yerr=stddevs)
                else:
                    stdev_args = dict()
                bars = axis[_idx].bar(
                    x_axis + offset,
                    founds,
                    width,
                    label=expected_category,
                    color=SelectivityBenchmarkPlot.colors[cat_id],
                    **stdev_args,
                )
                if add_values:
                    axis[_idx].bar_label(
                        bars, fmt="%.2f", fontsize=6, rotation=60, padding=3
                    )

                if len(slowdowns):
                    assert len(slowdowns) == len(x_axis)
                    slowdown_axis[_idx].scatter(
                        x_axis,
                        slowdowns,
                        label=expected_category,
                        color=SelectivityBenchmarkPlot.colors[cat_id],
                    )
            axis[_idx].set_xticks(
                x_axis + width * len(plot_order) / 2,
                [tup[0] for tup in category_sorted],
            )
            slowdown_axis[_idx].set_xticks(
                x_axis,
                [tup[0] for tup in category_sorted],
            )
            axis[_idx].set_title(f"Query: {query}")
            slowdown_axis[_idx].set_title(f"Query: {query}")
            if use_log:
                axis[_idx].set_yscale("log", base=10)
            slowdown_axis[_idx].set_yscale("log", base=10)
        axis[-1].legend(prop=dict(size=8), bbox_to_anchor=bbox_to_anchor)
        slowdown_axis[-1].legend(prop=dict(size=8), bbox_to_anchor=bbox_to_anchor)
        for ax in axis:
            ax.set(xlabel="Selectivity (%)", ylabel="Execution time (s)")
        for ax in slowdown_axis:
            ax.set(xlabel="Selectivity (%)", ylabel="Slowdown")
        figure.savefig(f"{directory}_{suffix}.png")
        slowdown_figure.savefig(f"{directory}_slowdown_{suffix}.png")


def main():
    bench_plotter = SelectivityBenchmarkPlot("selectity_microbenchmark")
    parsed = bench_plotter.parser.parse_args()
    all_results = []
    for file in parsed.files:
        complete_path = f"{parsed.root}/{file}/main_result.json"
        paths = glob.glob(complete_path)
        for path in paths:
            with open(path) as f:
                main_result = json.loads(f.read())
            print("checking:", path)
            bench_plotter.sanity_checks(main_result)
            all_results.append(main_result["result"])
    combined_results = reduce(merge_results, all_results)
    with open("combined.json", "w") as f:
        f.write(json.dumps(combined_results, indent=4))

    plotables = get_plotables(combined_results)
    # print(plotables)
    with open("test_py_out.tmp.py", "w") as f:
        f.write(repr(plotables))

    plot_plotables(plotables, PLOT_ORDER_SELECTIVITY_BENCH, "all")
    plot_plotables(
        plotables,
        PLOT_ORDER_RESTRICTED_SELECTIVITY_BENCH,
        "materialize",
        add_values=True,
        width=0.12,
    )
    plot_plotables(
        plotables,
        PLOT_ORDER_TRACEPROV,
        "traceprov",
        use_log=False,
        add_values=True,
        width=0.15,
        bbox_to_anchor=(1, 1),
    )
    # print(combined_results)


if __name__ == "__main__":
    main()
