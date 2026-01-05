from typing import Any, Tuple
from traceprovpy.tools.plot_utils import BenchmarkPlot, Plotable
import glob
import json
from functools import reduce
import matplotlib.pyplot as plt

COMPRESSIONS = [
    "uncompressed",
    "snappy",
    "gzip",
    "zstd",
    "brotli",
    # needs to be before lz4...
    "lz4_raw",
    "lz4",
    "native",
]

SCHEME_ORDER = [
    "uncompressed",
    "snappy",
    "gzip",
    "zstd",
    "brotli",
    "lz4_raw",
    "lz4",
    "native",
    "traceprov",
]


def assert_len_one(array):
    assert len(array) == 1
    if isinstance(array[0], list):
        return assert_len_one(array[0])
    else:
        return array[0]


def merge_items_strict(left: dict, right: dict) -> dict:
    merged = {**left, **right}
    assert (len(left) + len(right)) == len(merged)
    return merged


def extract_path(path: str):
    path_split = path.split("/")
    print(path_split)
    assert len(path_split) == 3
    path_name = path_split[1]
    assert "local_test_duckdb_analyze_" in path_name
    path_name = path_name.replace("local_test_duckdb_analyze_", "")
    for compression in COMPRESSIONS:
        if path_name.startswith(compression):
            return compression
    return Exception("got unrecognized type!")


def distribute_by_scale(previous: dict, current_pack: Tuple[str, dict]):
    current = current_pack[1]
    key = current_pack[0]
    return {
        **previous,
        **{
            scale: {
                **previous.get(scale, {}),
                key: merge_items_strict(previous.get(scale, {}).get(key, {}), value),
            }
            for (scale, value) in current.items()
        },
    }


def make_assert_in(expected_str: str):
    def _assert_in(in_str: str):
        assert expected_str in in_str
        return in_str

    return _assert_in


def group_by_values(previous: dict, current: dict):
    return {
        **previous,
        **{key: [*previous.get(key, []), value] for (key, value) in current.items()},
    }


def extract_duckdb_timings(val: Any):
    assert isinstance(val, list)
    cpp_results = [values["result_mode_cpp"] for values in val]
    return reduce(group_by_values, cpp_results, dict())


def extract_selectivity_duckdb(duckdb_results: dict):
    token = "DUCKDB_INFERENCE_"
    asserter = make_assert_in(token)
    return {
        asserter(key).replace(token, ""): extract_duckdb_timings(value)
        for (key, value) in duckdb_results.items()
    }


def extract_traceprov_timings(val: Any):
    assert isinstance(val, list)
    tp_timings = [
        dict(time=values["traceprov_infer_time"]["captured"][0][0]) for values in val
    ]
    return reduce(group_by_values, tp_timings, dict())


def extract_selectivity_traceprov(tp_results: dict):
    token = "traceprov_"
    asserter = make_assert_in(token)
    return {
        asserter(key).replace(token, ""): extract_traceprov_timings(value["extras"])
        for (key, value) in tp_results.items()
    }


def distribute_by_tp(distributed: dict):
    return {
        scale: {
            **{
                cat: extract_selectivity_duckdb(
                    {
                        infer_type: infer_results
                        for (infer_type, infer_results) in cat_value.items()
                        if not infer_type.startswith("traceprov")
                    }
                )
                for (cat, cat_value) in categories.items()
            },
            "traceprov": extract_selectivity_traceprov(
                {
                    # this arbitrarily takes any traceprov matching (with that selectivity.)
                    infer_type: infer_results
                    for cat_value in categories.values()
                    for (infer_type, infer_results) in cat_value.items()
                    if infer_type.startswith("traceprov")
                }
            ),
        }
        for (scale, categories) in distributed.items()
    }


import statistics


def simple_timings(result: dict):
    return {
        scale: {
            infer_type: {
                sel: statistics.median(sel_result["time"])
                for (sel, sel_result) in infer_result.items()
            }
            for (infer_type, infer_result) in category.items()
        }
        for (scale, category) in result.items()
    }


def simple_file_sizes(result: dict):
    return {
        scale: {
            infer_type: {
                sel: (
                    0
                    if infer_type == "traceprov"
                    else statistics.median(
                        [sum(single.values()) for single in sel_result["file_sizes"]]
                    )
                )
                for (sel, sel_result) in infer_result.items()
            }
            for (infer_type, infer_result) in category.items()
        }
        for (scale, category) in result.items()
    }


import numpy as np

width = 0.1


def tp_plot(scale: str, scale_results: dict, out_dir: str, size_results: dict):
    selectivity_set = set()
    # fig, ax = plt.subplots(layout="constrained")
    fig, (ax, ax_file_size) = plt.subplots(1, 2, figsize=(15, 6))
    fig.suptitle(f"Selectivity ({scale})")
    for idx, category in enumerate(
        sorted(list(scale_results.keys()), key=lambda x: SCHEME_ORDER.index(x))
    ):
        category_result: dict = scale_results[category]
        size_result = size_results[category]
        size = len(category_result.keys())
        if len(selectivity_set) and size not in selectivity_set:
            raise Exception("Expected same selectivity count!")

        selectivity_set.add(size)
        selectivity_timings = sorted(
            list(category_result.items()), key=lambda x: float(x[0])
        )
        selectivity_sizes = sorted(list(size_result.items()), key=lambda x: float(x[0]))
        x_axis = np.arange(size)
        x_axis_values = [sel for (sel, _) in selectivity_timings]
        rects = ax.bar(
            x_axis + width * idx,
            [value for (_, value) in selectivity_timings],
            width,
            label=category,
        )
        rects2 = ax_file_size.bar(
            x_axis + width * idx,
            [value for (_, value) in selectivity_sizes],
            width,
            label=category,
        )
    ax_file_size.set_xticks(x_axis + width, x_axis_values)
    ax_file_size.set(xlabel="Query", ylabel="Total size (bytes)")
    ax_file_size.legend(
        loc="center right", prop=dict(size=8), bbox_to_anchor=(1.25, 0.5)
    )
    ax_file_size.set_yscale("log", base=10)
    ax.set_yscale("log", base=10)
    ax.set_xticks(x_axis + width, x_axis_values)
    ax.set(xlabel="Selectivity (%)", ylabel="Execution time (microseconds)")
    fig.savefig(f"{out_dir}/scale_{scale}.png")


import os


def main():
    bench_plotter = BenchmarkPlot("duckdb_inference")
    bench_plotter.parser.add_argument("-o", "--out_dir", required=True)
    parsed = bench_plotter.parser.parse_args()
    assert os.system(f"rm -rf {parsed.out_dir}/") == 0
    assert os.system(f"mkdir -p {parsed.out_dir}") == 0
    combined = []
    for file in parsed.files:
        print(file)
        complete_path = f"{parsed.root}/{file}/main_result.json"
        paths = glob.glob(complete_path)
        print(paths)
        for path in paths:
            category = extract_path(path)
            print(path, category)
            with open(path) as f:
                main_result = json.loads(f.read())["result"]
            combined = [
                *combined,
                (
                    category,
                    {
                        scale: value["predicate_post"]
                        for scale, value in main_result.items()
                    },
                ),
            ]

    # Distribute by scale.
    distributed = reduce(distribute_by_scale, combined, {})
    distributed_by_tp = distribute_by_tp(distributed)
    with open("distributed.json", "w") as f:
        f.write(json.dumps(distributed_by_tp, indent=4))

    timings = simple_timings(distributed_by_tp)
    file_sizes = simple_file_sizes(distributed_by_tp)
    with open("distributed_time.json", "w") as f:
        f.write(json.dumps(timings, indent=4))

    with open("distributed_file_size.json", "w") as f:
        f.write(json.dumps(file_sizes, indent=4))

    for scale, scale_results in timings.items():
        tp_plot(scale, scale_results, parsed.out_dir, file_sizes[scale])


if __name__ == "__main__":
    main()
