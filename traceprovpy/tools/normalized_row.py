from collections import defaultdict
from functools import reduce
import math
from pathlib import Path
from typing import Iterable

import re

from matplotlib import pyplot as plt
import numpy as np

from traceprovpy.tools.plot_utils import BenchmarkPlot


class Extendable(object):
    inner: list[dict]

    def __init__(self, inner) -> None:
        self.inner = inner

    def add_key(self, outer_key: str):
        return [
            add_keys(outer_key, item)
            for item in self.inner
        ]

def add_keys(prefix, obj):
    if not isinstance(obj, dict): return obj
    return {
        f"{prefix}_{key}": value for (key, value) in obj.items()
    }

def merge(multiples: Iterable[dict]) -> dict:
    return reduce(lambda prev, curr: ({**prev, **curr}), multiples, dict())


NORM_ITER_COL = "iter"


class Normalizable(object):
    def __init__(self, **kwargs):
        keys = set(self.keys())
        in_keys = set(kwargs.keys())
        check_keys = in_keys
        if NORM_ITER_COL in in_keys:
            check_keys = set(check_keys)
            check_keys.remove(NORM_ITER_COL)
        assert keys == check_keys, f"Got different: {keys.symmetric_difference(in_keys)}"
        for key, value in kwargs.items():
            setattr(self, key, value)

    def keys(self):
        raise Exception("Expected to be implemented")

    def normalize(self, preserve_null=False) -> list[dict]:
        keys = self.keys() | {NORM_ITER_COL}
        extendables = [
            getattr(self, key).add_key(key)
            for key in keys
            if hasattr(self, key) and isinstance(getattr(self, key), Extendable)
        ]
        extended = list(map(merge, zip(*extendables, strict=True)))
        iter_count = list(extended)
        simple_keys = {
            key: getattr(self, key)
            for key in keys
            if hasattr(self, key)
            and not isinstance(getattr(self, key), Extendable)
            and (preserve_null or getattr(self, key) is not None)
        }
        truly_simple_keys = {key: value for (key, value) in simple_keys.items() if not isinstance(value, dict)}
        expended_values = merge([add_keys(key, value) for (key, value) in simple_keys.items() if isinstance(value, dict)])
        simple_keys = {**truly_simple_keys, **expended_values}
        if len(extended) == 0:
            return [simple_keys]
        rows = [
            merge([simple_keys, extended_cell, {NORM_ITER_COL: _idx}])
            for _idx, extended_cell in enumerate(extended, start=1)
        ]
        return rows


class NormalizedRow(Normalizable):
    category: str
    base: Extendable
    capture: Extendable
    base_profile: Extendable
    capture_profile: Extendable

    def keys(self):
        return {"category", "base", "capture", "base_profile", "capture_profile"}


def extract_bucket_category(file_name: str):
    if "no_partition" in file_name:
        bucket = None
    else:
        match = re.search(r"(\d+)_partition", file_name)
        assert match is not None
        bucket = match.groups()[0]
    return bucket


def try_match(file_name: str, hypen: bool):
    sep = "-" if hypen else "_"
    match = re.search(rf"_thread{sep}(\d+)", file_name)
    if match is None:
        match = re.search(rf"_threads{sep}(\d+)", file_name)
    return match


def extract_thread_category(file_name: str):
    match = try_match(file_name, True)
    if match is None:
        match = try_match(file_name, False)
    assert match is not None
    bucket = int(match.groups()[0])
    return str(bucket)


import statistics


def tap_simple_result(result: dict):
    return dict(
        time=result["time"], width=result["width"], row_count=result["row_count"]
    )


def make_dummy_simple_result(time, width=-1, row_count=-1):
    return dict(time=time, width=width, row_count=row_count)


def tap_profile_result(result: dict):
    # to handle old profile results.
    return dict(latency=result["latency"] if "latency" in result else result["timing"])


def make_dummy_profile_result(time):
    return dict(latency=time)


def sum_simple_result(left_result: dict, right_result: dict):
    assert isinstance(left_result, dict) and isinstance(right_result, dict)
    return {
        **left_result,
        **{key: left_result.get(key) + value for (key, value) in right_result.items()},
    }


def combine_tap_result(results: list[dict]):

    def _reduce(previous, current):
        return [sum_simple_result(*res) for res in zip(previous, current)]

    try:
        return reduce(_reduce, results[1:], results[0])
    except:
        return None


def extract_traceprov(traceprov_result: dict):
    return dict(
        base=Extendable(map(tap_simple_result, traceprov_result["base_time"])),
        capture=Extendable(map(tap_simple_result, traceprov_result["capture_time"])),
        base_profile=Extendable(
            map(tap_profile_result, traceprov_result["base_profile"])
        ),
        capture_profile=Extendable(
            map(tap_profile_result, traceprov_result["capture_profile"])
        ),
    )


def extract_infer(infer_results: dict):
    return combine_tap_result(
        [
            (
                [
                    (
                        {
                            **tap_profile_result(node),
                            **tap_simple_result(infer_result["times"][node_idx]),
                        }
                    )
                    for node_idx, node in enumerate(infer_result["profile"])
                ]
            )
            for infer_result in infer_results
        ]
    )


def extract_stats_sample_infer_row(sql_map: list[dict], time_results: list[dict]):
    time_map = defaultdict(dict)
    for cell, time_cell in zip(sql_map, time_results, strict=True):
        key = tuple(cell[0])
        assert len(key) == 3
        element_id, *rest = key
        rest = tuple(rest)
        time_map[rest][element_id] = [
            *time_map[rest].get(element_id, []),
            time_cell["time"],
        ]
    time_map_summed = []
    # print(time_map)
    for key, values in time_map.items():
        # print("merging: ", values.keys())
        # print(list(zip(*values.values(), strict=True)))
        time_map_summed.append(list(map(sum, zip(*values.values(), strict=True))))
    # print(time_map_summed)
    # time_map_reduced = {key: sorted(values[5:], reverse=True)[3:] for (key, values) in time_map.items()}
    time_map_reduced = [
        sorted(values[5:], reverse=True)[2:] for values in time_map_summed
    ]
    # time_map_reduced = {key: values for (key, values) in time_map.items()}
    time_map_reduced = [
        (
            statistics.median(values),
            statistics.stdev(values) / statistics.mean(values),
        )
        for values in time_map_reduced
    ]
    stats = dict(
        average_time=statistics.mean(item[0] for item in time_map_reduced),
        max_stdev_ratio=max(item[1] for item in time_map_reduced),
    )
    return stats


class NormalizedSampleInferRow(NormalizedRow):
    average_time: float
    max_stdev_ratio: float
    max_stdev: float
    mean_stdev: float
    min_backtrace_time: float
    max_backtrace_time: float
    std_backtrace_time: float
    sum_backtrace_time: float

    def keys(self):
        return super().keys() | {
            "average_time",
            "max_stdev_ratio",
            "max_stdev",
            "min_backtrace_time",
            "max_backtrace_time",
            "std_backtrace_time",
            "sum_backtrace_time",
            "mean_stdev",
        }


class NormalizedInferRow(NormalizedRow):
    infer: Extendable

    def keys(self):
        return super().keys() | {"infer"}


category_order = [
    "TraceProv(bucket: None)",
    "TraceProv(bucket: 2)",
    "TraceProv(bucket: 4)",
    "TraceProv(bucket: 8)",
    "TraceProv(bucket: 16)",
]


def plot_partition_result(
    x_axis_values,
    x_axis_labels,
    out_dir: Path,
    remap_data,
    x_axis_item,
    figure_title,
    capture_figure_title,
    width=0.1,
    new_width=0.08,
    prefix="",
):
    fig, (ax, infer_time) = plt.subplots(1, 2, figsize=(20, 6))
    fig.suptitle(figure_title)
    capture_infer, capture_infer_ax = plt.subplots(1, 1, figsize=(15, 6))
    capture_infer.suptitle(capture_figure_title)

    fig_std_dev, std_dev_ax = plt.subplots(1, 3, figsize=(15, 6))
    fig_std_dev.suptitle(figure_title)

    x_axis = np.arange(len(x_axis_values))
    extra_id = 0
    for category_idx, (category, category_data) in enumerate(
        sorted(remap_data.items(), key=lambda x: category_order.index(x[0]))
    ):
        y_values = [
            100
            * (
                (category_data[x]["capture_median"] / category_data[x]["base_median"])
                - 1
            )
            for x in x_axis_values
        ]
        x_axis_adjusted = x_axis + (width * category_idx)
        ax.bar(
            x_axis_adjusted,
            y_values,
            width=width,
            label=category,
            color=BenchmarkPlot.colors[category_idx],
        )
        get_key_value = lambda in_key: np.array(
            [category_data[x][in_key] for x in x_axis_values]
        )
        time_values = get_key_value("infer_time")
        infer_time.bar(
            x_axis_adjusted,
            time_values,
            width=width,
            label=category,
            color=BenchmarkPlot.colors[category_idx],
        )
        std_dev_ax[0].bar(
            x_axis + (new_width * (category_idx)),
            get_key_value("base_variance") * 100,
            width=new_width,
            label=f"{category}_base",
            color=BenchmarkPlot.colors[category_idx],
        )
        std_dev_ax[0].set_title("Base")

        std_dev_ax[1].bar(
            x_axis + (new_width * (category_idx + 1)),
            get_key_value("capture_variance") * 100,
            width=new_width,
            label=f"{category}_capture",
            color=BenchmarkPlot.colors[category_idx],
        )
        std_dev_ax[1].set_title("Capture")

        std_dev_ax[2].bar(
            x_axis + (new_width * (category_idx + 2)),
            get_key_value("infer_variance") * 100,
            width=new_width,
            label=f"{category}",
            color=BenchmarkPlot.colors[category_idx],
        )
        std_dev_ax[2].set_title("Infer")

        # capture_infer_ax.bar(
        #     x_axis + (extra_id * new_width),
        #     get_key_value("base_median"),
        #     width=new_width,
        #     label=f"{category}_base",
        # )
        # extra_id += 1
        # capture_infer_ax.bar(
        #     x_axis + (extra_id * new_width),
        #     get_key_value("capture_median"),
        #     width=new_width,
        #     label=f"{category}_capture",
        # )
        # extra_id += 1
        capture_infer_ax.bar(
            x_axis + (extra_id * new_width),
            get_key_value("capture_median") + time_values,
            width=new_width,
            label=f"{category} (F + I)",
        )
        extra_id += 1

    ax.set_xticks(x_axis + (len(remap_data) / 2) * width, x_axis_labels)
    ax.set_ylabel("Relative Overhead (%)")

    infer_time.set_xticks(x_axis + (len(remap_data) / 2) * width, x_axis_labels)
    infer_time.set_ylabel("Infer Time (microseconds)")
    infer_time.set_yscale("log")

    for _id, _std_dev_ax in enumerate(std_dev_ax):
        _std_dev_ax.set_xticks(
            x_axis + (len(remap_data) / 2) * new_width, x_axis_labels
        )
        if _id == len(std_dev_ax) - 1:
            _std_dev_ax.legend(prop=dict(size=7), bbox_to_anchor=(0.93, 0.50))
        if _id == 0:
            _std_dev_ax.set_ylabel("Standard Deviation / Mean %")
        _std_dev_ax.set_xlabel(x_axis_item)

    capture_infer_ax.set_xticks(
        x_axis + (len(remap_data) / 2) * new_width,
        x_axis_labels,
    )

    ax.set_xlabel(x_axis_item)
    capture_infer_ax.set_ylabel("Capture + Infer Time (microsecond)")
    capture_infer_ax.set_xlabel(x_axis_item)
    infer_time.set_xlabel(x_axis_item)

    ax.legend(prop=dict(size=8), bbox_to_anchor=(-0.05, 1.1))
    capture_infer_ax.legend(prop=dict(size=7), bbox_to_anchor=(0, 1.1))
    fig.savefig((out_dir / f"{prefix}_relative_ovh.png").as_posix())
    capture_infer.savefig((out_dir / f"{prefix}_time.png").as_posix())
    fig_std_dev.savefig((out_dir / f"{prefix}_stddev.png").as_posix())


# basically flattens keys into individual values.
def _reduce_keys(prev: dict, curr: dict):
    return {
        **prev,
        **{key: [*prev.get(key, []), value] for (key, value) in curr.items()},
    }


def remove_throwaway(in_values: list):
    assert len(in_values) == 15
    return in_values[5:]


def aggregate_result(results: list[dict]):
    flattened = reduce(_reduce_keys, results, dict())
    median_values = {
        f"{key}_median": statistics.median(remove_throwaway(values))
        for (key, values) in flattened.items()
    }
    stdev_ratio_values = {
        f"{key}_std_ratio": statistics.stdev(remove_throwaway(values))
        / statistics.mean(remove_throwaway(values))
        for (key, values) in flattened.items()
    }
    std_values = {
        f"{key}_std": statistics.stdev(remove_throwaway(values))
        for (key, values) in flattened.items()
    }
    agg = {**median_values, **stdev_ratio_values, **std_values}
    return agg


def aggregate_over_samples(results: list[dict]):
    return dict(
        average_time=statistics.mean(result["latency_median"] for result in results),
        min_backtrace_time=min(result["latency_median"] for result in results),
        max_backtrace_time=max(result["latency_median"] for result in results),
        std_backtrace_time=(
            None
            if len(results) == 1
            else statistics.stdev(result["latency_median"] for result in results)
        ),
        max_stdev_ratio=max(result["latency_std_ratio"] for result in results),
        max_stdev=max(result["latency_std"] for result in results),
        mean_stdev=math.sqrt(
            sum(result["latency_std"] ** 2 for result in results) / len(results)
        ),
        sum_backtrace_time=sum(result["latency_median"] for result in results),
    )


def parse_sample_results(sample_infer_result, capture_indexes, is_traceprov):
    sql_spec_map = sample_infer_result["sql_spec_map"]
    sample_result = dict()
    for profile_entry_idx, profile_entry in enumerate(sample_infer_result["profile"]):
        if profile_entry_idx in capture_indexes:
            last_capture_result = sample_infer_result["result_time"][profile_entry_idx]
            continue
        first_key, iter_id = sql_spec_map[profile_entry_idx]
        assert len(first_key) in (2, 3)
        if len(first_key) == 2:
            part_key = first_key
        else:
            part_key = first_key[1:]

        # print("using keys: ", part_key, first_key)
        part_key = tuple(part_key)
        post_process_time = 0
        if part_key not in sample_result:
            sample_result[part_key] = [list() for _ in range(len(capture_indexes))]
            if is_traceprov:
                extra_item = sample_infer_result["result_time"][profile_entry_idx][
                    "option"
                ]["extra"][0]
                extra_item = extra_item.replace("[", "").replace("]", "")
                extra_item_split = extra_item.split(",")[-1]
                if "partition_time" in extra_item_split:
                    part_time = int(extra_item_split.split("-")[-1])
                else:
                    part_time = 0
                print(last_capture_result["option"])
                sql_time = last_capture_result["option"].get(
                    "misc_key_value_sql_compilation_time", 0
                )
                # Need to include the post-process time just once per offset.
                # This is very very unfair to our system
                # Since we let SmokedDuck get away with doing post process per query, but we're still faster ;)
                print("SQL time: ", sql_time)
                post_process_time = (part_time + sql_time) / (10 ** (-6))
        sample_result[part_key][iter_id].append(
            sum_simple_result(
                tap_profile_result(profile_entry), dict(latency=post_process_time)
            )
        )
    # need to reduce the result up a bit
    sample_result_reduced = aggregate_over_samples(
        [
            aggregate_result(
                [
                    reduce(sum_simple_result, iter_result)
                    for iter_result in sample_results
                ]
            )
            for sample_results in (sample_result.values())
        ]
    )
    return sample_result, sample_result_reduced
