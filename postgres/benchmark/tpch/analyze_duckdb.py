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
    assert len(path_split) == 5
    path_name = path_split[3]
    assert "local_test_duckdb_analyze_" in path_name
    path_name = path_name.replace("local_test_duckdb_analyze_", "")
    for compression in COMPRESSIONS:
        if path_name.startswith(compression):
            return compression
    return Exception("got unrecognized type!")


def remove_key(in_dict: dict, key: str):
    assert key in in_dict
    return {left_key: value for (left_key, value) in in_dict.items() if left_key != key}


def filter_empty(in_dict: dict):
    return {key: value for (key, value) in in_dict.items() if len(value) > 0}


def distribute_by_params(previous: dict, current_pack: Tuple[str, dict]):
    key = current_pack[0]
    current = current_pack[1]
    return {
        **previous,
        **{
            param: {
                **previous.get(param, {}),
                key: filter_empty(
                    {
                        qnum: remove_key(qvalue, "base")
                        for (qnum, qvalue) in param_values.items()
                    }
                ),
            }
            for (param, param_values) in current.items()
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
        dict(time=values["traceprov_infer_time_0"]["captured"][0][0]) for values in val
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
        params: {
            **{
                infer_type: {
                    qnum: extract_duckdb_timings(
                        list(remove_key(qnum_data, "traceprov").values())[0]
                    )
                    for (qnum, qnum_data) in query_data.items()
                }
                for (infer_type, query_data) in param_values.items()
            },
            "traceprov": {
                **param_values.get("traceprov", {}),
                **{
                    qnum: extract_traceprov_timings(qnum_data["traceprov"]["extras"])
                    for query_data in param_values.values()
                    for qnum, qnum_data in query_data.items()
                },
            },
        }
        for (params, param_values) in distributed.items()
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


import numpy as np

width = 0.1


def tp_plot(scale: str, scale_results: dict):
    selectivity_set = set()
    fig, ax = plt.subplots(layout="constrained")
    for idx, category in enumerate(
        sorted(list(scale_results.keys()), key=lambda x: SCHEME_ORDER.index(x))
    ):
        category_result: dict = scale_results[category]
        size = len(category_result.keys()) - 1
        if len(selectivity_set) and size not in selectivity_set:
            raise Exception("Expected same selectivity count!")

        selectivity_set.add(size)
        selectivity_timings = sorted(
            list(category_result.items()), key=lambda x: int(x[0])
        )[1:]
        x_axis = np.arange(size)
        x_axis_values = [sel for (sel, _) in selectivity_timings]
        rects = ax.bar(
            x_axis + width * idx,
            [value for (_, value) in selectivity_timings],
            width,
            label=category,
        )
    ax.legend(ncols=3)
    ax.set_xticks(x_axis + width, x_axis_values)
    fig.savefig(f"param_{scale}.png")


def main():
    bench_plotter = BenchmarkPlot("duckdb_inference")
    parsed = bench_plotter.parser.parse_args()
    combined = []
    for file in parsed.files:
        print(file)
        complete_path = f"{parsed.root}/{file}/main_result.json"
        paths = glob.glob(complete_path)
        for path in paths:
            category = extract_path(path)
            print(path, category)
            with open(path) as f:
                main_result = json.loads(f.read())["result"]
            combined = [
                *combined,
                (category, main_result),
            ]

    # Distribute by scale.
    distributed = reduce(distribute_by_params, combined, {})
    distributed_by_tp = distribute_by_tp(distributed)
    with open("distributed.json", "w") as f:
        # sampled = {
        #     key: value
        #     for (idx, (key, value)) in enumerate(distributed_by_tp.items())
        #     if idx < 2
        # }
        f.write(json.dumps(distributed_by_tp, indent=4))

    timings = simple_timings(distributed_by_tp)
    with open("distributed_time.json", "w") as f:
        f.write(json.dumps(timings, indent=4))

    for scale, scale_results in timings.items():
        tp_plot(scale, scale_results)


if __name__ == "__main__":
    main()
