# analyzes the TPC-H result data.
import glob
import json
import os
from pathlib import Path

from matplotlib import pyplot as plt
import numpy as np
from traceprovpy.tools.file_utils import json_read_file, just_write
from traceprovpy.tools.plot_utils import BenchmarkPlot
import statistics


def handle_q15_data(query_data):
    base_data = [
        el["base"][0]["explain_time"]
        for el in (query_data["base_15_skippable"]["extras"])
    ]
    traceprov_data = [
        el["traceprov"][0]["explain_time"]
        for el in (query_data["traceprov_15_skippable"]["extras"])
    ]
    traceprov_infer_data = tap_extra(query_data["traceprov_15_skippable"]["extras"])
    return dict(
        base=base_data,
        traceprov=traceprov_data,
        traceprov_infer_data=traceprov_infer_data["time"],
        traceprov_infer_row_count=traceprov_infer_data["row_counts"],
    )


def tap_result(result: list[dict]):
    return [b["explain_time"] for b in result]


def maybe_core_results(in_dict: dict | list):
    if isinstance(in_dict, list):
        return in_dict
    return in_dict.get("core_results")


def tap_extra(extra_result: list[dict]):
    res_results = [
        sum(
            [
                res_item["raw_results"][-1]["result"]["explain_time"]
                for res_item in maybe_core_results(res["traceprov_infer"][0])
            ]
        )
        for res in extra_result
    ]
    row_counts = [
        [
            res_item["row_count"]
            for res_item in maybe_core_results(res["traceprov_infer"][0])
        ]
        for res in extra_result
    ]
    return dict(time=res_results, row_counts=row_counts)


def handle_data(query, query_data):
    print(query)
    if query == "15":
        return handle_q15_data(query_data)

    base_data = tap_result(query_data["base"]["base"])
    traceprov_data = tap_result(query_data["traceprov"]["base"])
    traceprov_extras = tap_extra(query_data["traceprov"]["extras"])

    print(base_data)
    print(traceprov_data)
    print(traceprov_extras)
    return dict(
        base=base_data,
        traceprov=traceprov_data,
        traceprov_infer_data=traceprov_extras["time"],
        traceprov_infer_row_count=traceprov_extras["row_counts"],
    )


SIMPLE_FRAC = lambda elem: (elem[0] / elem[1])


def add_extra_data(query_data):
    query_data["traceprov_capture_infer"] = list(
        map(
            sum,
            zip(
                query_data["traceprov"],
                query_data["traceprov_infer_data"],
                strict=True,
            ),
        )
    )
    query_data["capture_slowdown"] = list(
        map(
            SIMPLE_FRAC,
            zip(query_data["traceprov"], query_data["base"]),
        )
    )
    query_data["capture_and_infer_slowdown"] = list(
        map(SIMPLE_FRAC, zip(query_data["traceprov_capture_infer"], query_data["base"]))
    )
    remapped = {**query_data}
    for key, measure in query_data.items():
        if key in ["traceprov_infer_row_count"]:
            continue
        mean_key = f"{key}_mean"
        median_key = f"{key}_median"
        stdev_key = f"{key}_stdev"
        extra_data = dict()
        extra_data[mean_key] = statistics.mean(measure)
        extra_data[median_key] = statistics.median(measure)
        extra_data[stdev_key] = statistics.stdev(measure)
        orig_len = len(remapped)
        remapped = {**remapped, **extra_data}
        # assert that there were no duplicates essentially.
        assert len(remapped) == orig_len + len(extra_data)
    return remapped


def analyze(result: dict, out_dir: Path):
    params_default = result["result"]["params_default"]
    remapped = {
        query: add_extra_data(handle_data(query, query_data))
        for query, query_data in params_default.items()
    }
    just_write(out_dir / "remapped.json", json.dumps(remapped))
    x_axis_value = list(map(str, range(1, 23)))
    get_key_data = lambda key: [(remapped[x][key]) for x in x_axis_value]
    capture_slowdown_data = get_key_data("capture_slowdown_median")
    capture_and_infer_slowdown = get_key_data("capture_and_infer_slowdown_median")
    capture_slowdown_error = get_key_data("capture_slowdown_stdev")
    capture_and_infer_slowdown_error = get_key_data("capture_and_infer_slowdown_stdev")
    base_time_stdev = [
        remapped[x]["base_stdev"] / remapped[x]["base_mean"] for x in x_axis_value
    ]
    capture_time_stdev = [
        remapped[x]["traceprov_stdev"] / remapped[x]["traceprov_mean"]
        for x in x_axis_value
    ]
    infer_time_stdev = [
        remapped[x]["traceprov_infer_data_stdev"]
        / remapped[x]["traceprov_infer_data_mean"]
        for x in x_axis_value
    ]
    base_time_data = get_key_data("base_median")
    base_time_stdev_data = get_key_data("base_stdev")

    traceprov_time_data = get_key_data("traceprov_median")
    traceprov_time_stdev_data = get_key_data("traceprov_stdev")

    traceprov_infer_time_data = get_key_data("traceprov_infer_data_median")
    traceprov_infer_time_stdev_data = get_key_data("traceprov_infer_data_stdev")

    slowdown_fig, slowdown_axis = plt.subplots(1, 1, figsize=(20, 6))
    stdev_fig, stdev_axis = plt.subplots(1, 1, figsize=(20, 6))
    infer_time_fig, infer_time_axis = plt.subplots(1, 1, figsize=(20, 6))
    x_axis = np.arange(len(x_axis_value))
    width = 0.15
    slowdown_axis.bar(
        x_axis,
        capture_slowdown_data,
        width=width,
        label="Forward Overhead",
        yerr=capture_slowdown_error,
    )
    slowdown_axis.bar(
        x_axis + width,
        capture_and_infer_slowdown,
        width=width,
        label="Forward + Infer Overhead",
        yerr=capture_and_infer_slowdown_error,
    )
    slowdown_axis.set_xticks(x_axis + 0.5 * width, x_axis_value)
    slowdown_axis.set_xlabel("Query")
    slowdown_axis.set_ylabel("Relative Overhead")
    slowdown_axis.legend(prop=dict(size=8))

    stdev_axis.bar(x_axis, base_time_stdev, width=width, label="Base")
    stdev_axis.bar(x_axis + width, capture_time_stdev, width=width, label="Capture")
    stdev_axis.bar(x_axis + 2 * width, infer_time_stdev, width=width, label="Infer")
    stdev_axis.set_xticks(x_axis + (1.5) * width, x_axis_value)
    stdev_axis.set_xlabel("Query")
    stdev_axis.set_ylabel("Std/Mean")
    stdev_axis.legend(prop=dict(size=8))

    infer_time_axis.bar(
        x_axis, base_time_data, width=width, label="Base", yerr=base_time_stdev_data
    )
    infer_time_axis.bar(
        x_axis + width,
        traceprov_time_data,
        width=width,
        label="Capture",
        yerr=traceprov_time_stdev_data,
    )
    infer_time_axis.bar(
        x_axis + 2 * width,
        traceprov_infer_time_data,
        width=width,
        label="Infer",
        yerr=traceprov_infer_time_stdev_data,
    )
    infer_time_axis.set_xticks(x_axis + (1.5) * width, x_axis_value)
    infer_time_axis.set_xlabel("Query")
    infer_time_axis.set_ylabel("Time(s)")
    infer_time_axis.legend(prop=dict(size=8))
    infer_time_axis.set_yscale("log", base=10)

    stdev_fig.savefig((out_dir / "stdev.png").as_posix())
    slowdown_fig.savefig((out_dir / "slowdown.png").as_posix())
    infer_time_fig.savefig((out_dir / "time.png").as_posix())


def main():
    parser = BenchmarkPlot("threads")
    parser.parser.add_argument("-sf", required=True)
    parsed = parser.parser.parse_args()

    all_results = []
    for file in parsed.files:
        complete_path = f"{parsed.root}/{file}/main_result.json"
        paths = glob.glob(complete_path)
        for path in paths:
            print(path)
            result: dict = json_read_file(path)
            all_results.append(result)
            out_dir = Path(parsed.out_dir) / parsed.sf
            os.makedirs(out_dir, exist_ok=True)
            analyze(result, out_dir)


if __name__ == "__main__":

    main()
