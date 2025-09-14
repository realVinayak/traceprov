import json
import statistics
from typing import Any, Dict, NamedTuple
import sys
import numpy as np
import matplotlib.pyplot as plt
import re

LOWER_BOUND = 25
UPPER_BOUND = 75

muller_data = """
8.00	1
1.50	2
4.00	3
8.00	4
1.50	5
6.00	6
1.25	7
3.00	8
6.00	9
6.00	10
200.00	11
3.00	12
20.00	13
6.00	14
100.00	17
1.00	19
"""


REGEX = r"^(\d+)\."


class HashQuery(NamedTuple):
    query: int
    query_raw: str
    label: str = None

    def add_label(self) -> "HashQuery":
        try:
            value = int(self.query_raw)
            return self._replace(label=str(value))
        except ValueError:
            # Clean stuff like .gprom. and .materialized. and .sql.
            SPECIAL_CHARS = [".gprom", ".materialized", ".sql"]
            value = self.query_raw
            for char in SPECIAL_CHARS:
                value = value.replace(char, "")
            if ".no_limit" in value:
                value = value.replace(".no_limit", "NL")
            return self._replace(label=value)

    @staticmethod
    def make_from_query(query_str: str) -> "HashQuery":
        try:
            parsed_query = int(query_str)
        except ValueError:
            parsed_query = int(re.findall(REGEX, query_str)[0])

        hash_q = HashQuery(query=parsed_query, query_raw=query_str)
        labeled = hash_q.add_label().label
        if "gprom" in hash_q.query_raw:
            query_str = labeled
            if query_str[0] == "0":
                query_str = query_str[1:]
        if ".no_limit" in query_str:
            query_str = query_str.replace(".no_limit", "NL")
        return HashQuery(query=parsed_query, query_raw=query_str)


muller_results = {
    HashQuery.make_from_query(q): float(slowdown)
    for (slowdown, q) in [
        tuple(row.split("\t")) for row in muller_data.split("\n") if len(row) > 1
    ]
}

print(muller_results)


def get_json_dump(files):
    data = {}

    for file in files:
        with open(file) as f:
            new_data = json.loads(f.read())
            for key in new_data:
                data[key] = {**data.get(key, {}), **new_data[key]}

    return data


def aggregate_over_params(data: Dict[str, Dict[str, Any]]):
    # Simply return the aggregate over the params
    agg_data = {}
    for _, per_dir in sorted(data.items(), key=lambda x: x[0]):
        for query_key, cells in per_dir.items():
            agg_data[query_key] = [*agg_data.get(query_key, []), *cells]

    return agg_data


def print_stats(name, data):
    for query, timings in data.items():
        print(name, query, len(timings))


def assert_correct_type(in_dict):
    for key in in_dict:
        assert isinstance(key, HashQuery)
    return in_dict


add_labels = lambda queries: [(query.add_label().label) for query in queries]


forward_times = lambda input_tuples: [i[0] for i in input_tuples]
infer_times = lambda input_tuples: [i[1] / 1000 for i in input_tuples]
forward_and_inference = lambda forwards, infers: [
    f + i for (f, i) in zip(forwards, infers)
]
forward_and_inference_and_material = lambda input_tuples: [
    i[0] + i[1] for i in input_tuples
]
num_records = lambda input_tuples: [sum(i[2]) for i in input_tuples]
num_records_gprom = lambda input_tuples: [sum(i[1]) for i in input_tuples]


def generate_figures(
    baseline_files,
    traceprov_files,
    traceprov_files_infer,
    gprom_files,
    params=None,
    label=None,
    use_muller=False,
    out_dir="./",
    split_traceprov=False,
):
    plt.clf()

    _baseline_data = get_json_dump(baseline_files)
    _traceprov_data = get_json_dump(traceprov_files)
    _traceprov_infer = get_json_dump(traceprov_files_infer)
    _gprom_data = get_json_dump(gprom_files)

    if params is None:
        baseline_data = aggregate_over_params(_baseline_data)
        traceprov_data = aggregate_over_params(_traceprov_data)
        traceprov_infer = aggregate_over_params(_traceprov_infer)
        gprom_data = aggregate_over_params(_gprom_data)
    else:
        baseline_data = _baseline_data[params]
        traceprov_data = _traceprov_data[params]
        traceprov_infer = _traceprov_infer[params]
        gprom_data = _gprom_data.get(params, {})

    gprom_data = {
        queries: [(value["duration"], value["num_records"]) for value in values]
        for queries, values in gprom_data.items()
    }

    # print_stats("baseline", baseline_data)
    # print_stats("traceprov", traceprov_data)
    # print_stats("traceprov_infer", traceprov_infer)

    # For each param (or set of params), there are two graphs.

    # First graph shows:
    # 1. Baseline
    # 2. Traceprov Forward
    # 3. Traceprov Inference
    # 4. Traceprov Forward + Traceprov Inference
    # 5. Traceprov Forward + Traceprov Inference + Materialization
    # NOTE: We also show the average number of records

    # Second graph shows:
    # 1. Slowdown using (Traceprov Forward + Traceprov Inference) / Baseline
    # 2. Slowdown using (Traceprov Forward + Traceprov Inference + Materialization) / Baseline

    get_tuple = lambda times: (
        np.median(times),
        np.percentile(times, LOWER_BOUND),
        np.percentile(times, UPPER_BOUND),
    )

    get_tuple_dump = lambda times: (
        statistics.median(times),
        statistics.variance(times),
    )

    raw_gprom_time = {
        HashQuery.make_from_query(query): times for (query, times) in gprom_data.items()
    }

    raw_baseline_time = {
        HashQuery.make_from_query(query): times
        for (query, times) in baseline_data.items()
    }
    baseline_time = {
        HashQuery.make_from_query(query): get_tuple(times)
        for query, times in baseline_data.items()
    }

    gprom_time = {
        HashQuery.make_from_query(query): get_tuple(forward_times(times))
        for query, times in gprom_data.items()
    }

    dump_gprom_time = {
        HashQuery.make_from_query(query): get_tuple_dump(forward_times(times))
        for query, times in gprom_data.items()
    }

    dump_baseline_time = {
        HashQuery.make_from_query(query): get_tuple_dump(times)
        for query, times in baseline_data.items()
    }
    raw_traceprov_forward_times = {
        HashQuery.make_from_query(query): forward_times(times)
        for query, times in traceprov_data.items()
    }

    traceprov_forward_time = {
        query: get_tuple(times) for query, times in raw_traceprov_forward_times.items()
    }

    raw_traceprov_infer_time = {
        HashQuery.make_from_query(query): infer_times(times)
        for query, times in traceprov_infer.items()
    }

    traceprov_infer_time = {
        query: get_tuple(times) for (query, times) in raw_traceprov_infer_time.items()
    }

    dump_forward_times = {
        query: get_tuple_dump(times)
        for query, times in raw_traceprov_forward_times.items()
    }

    dump_infer_times = {
        query: get_tuple_dump(times)
        for (query, times) in raw_traceprov_infer_time.items()
    }

    if len(raw_traceprov_forward_times) != len(raw_traceprov_infer_time):
        print(
            "[debug]:",
            "Mismatch in sizes for inference ",
            len(raw_traceprov_forward_times),
            len(raw_traceprov_infer_time),
        )
        print(
            "[debug]:",
            "Mismatch in sizes for inference",
            set(raw_traceprov_forward_times.keys()).symmetric_difference(
                set(raw_traceprov_infer_time.keys())
            ),
        )

    # assert len(raw_traceprov_forward_times) == len(raw_traceprov_infer_time)

    raw_traceprov_forward_and_infer = {
        query: forward_and_inference(
            raw_traceprov_forward_times[query],
            raw_traceprov_infer_time.get(
                query, [0] * len(raw_traceprov_forward_times[query])
            ),
        )
        for query in raw_traceprov_forward_times.keys()
    }

    traceprov_forward_and_infer = {
        query: get_tuple(times)
        for query, times in raw_traceprov_forward_and_infer.items()
    }

    dump_traceprov_forward_and_infer = {
        query: get_tuple_dump(times)
        for query, times in raw_traceprov_forward_and_infer.items()
    }

    raw_traceprov_forward_and_infer_and_material = {
        HashQuery.make_from_query(query): forward_and_inference_and_material(tuples)
        for query, tuples in traceprov_data.items()
    }

    dump_traceprov_forward_and_infer_and_material = {
        query: get_tuple_dump(times)
        for query, times in raw_traceprov_forward_and_infer_and_material.items()
    }

    traceprov_forward_and_infer_and_material = {
        query: get_tuple(times)
        for query, times in raw_traceprov_forward_and_infer_and_material.items()
    }

    traceprov_logged_records = {
        HashQuery.make_from_query(query): get_tuple(num_records(tuples))
        for (query, tuples) in traceprov_data.items()
    }

    dump_traceprov_logged_records = {
        HashQuery.make_from_query(query): get_tuple_dump(num_records(tuples))
        for (query, tuples) in traceprov_data.items()
    }

    dump_gprom_logged_records = {
        HashQuery.make_from_query(query): get_tuple_dump(num_records_gprom(tuples))
        for (query, tuples) in gprom_data.items()
    }

    # print(baseline_time)
    # print("\n\n")
    # print(traceprov_forward_time)
    # print("\n\n")
    # print(traceprov_infer_time)
    # print("\n\n")
    # print(traceprov_forward_and_infer)
    # print("\n\n")
    # print(traceprov_forward_and_infer_and_material)
    # print("\n\n")
    # print(traceprov_logged_records)

    queries = list(
        enumerate(
            list(sorted(list(baseline_time.keys()), key=lambda x: x.query)), start=1
        )
    )

    reverse_query_mapping = {query: index for (index, query) in queries}
    assert len(reverse_query_mapping) == len(queries)

    # assert len(assert_correct_type(baseline_time)) == len(traceprov_forward_time)
    # assert len(assert_correct_type(traceprov_forward_time)) == len(traceprov_infer_time)
    # assert len(assert_correct_type(traceprov_infer_time)) == len(
    #     traceprov_forward_and_infer
    # )
    # assert len(assert_correct_type(traceprov_forward_and_infer)) == len(
    #     assert_correct_type(traceprov_forward_and_infer_and_material)
    # )

    if split_traceprov:

        group_width = 0.85
        num_groups = 6
        bar_width = group_width / num_groups
        group_offsets = np.linspace(
            -(num_groups - 1) / 2 * bar_width,
            (num_groups - 1) / 2 * bar_width,
            num_groups,
        )

        fig, (ax, ax_table) = plt.subplots(1, 2, figsize=(15, 6))

        ax.bar(
            [idx + group_offsets[0] for idx, _ in queries],
            [baseline_time[query][0] for _, query in queries],
            width=bar_width,
            label="Baseline",
        )

        # print(gprom_time)
        # for _, q in queries:
        #     print(len(gprom_time))
        #     print("searching for", q in gprom_time)

        # gprom_graph_data = [gprom_time.get(query, (0, 0))[0] for _, query in queries]
        # print(gprom_graph_data)
        ax.bar(
            [idx + group_offsets[1] for idx, _ in queries],
            [gprom_time.get(query, (0, 0))[0] for _, query in queries],
            width=bar_width,
            label="GProM (Join)",
        )

        ax.bar(
            [idx + group_offsets[2] for idx, _ in queries],
            [traceprov_forward_time[query][0] for _, query in queries],
            width=bar_width,
            label="Trace",
        )
        ax.bar(
            [idx + group_offsets[3] for idx, _ in queries],
            [traceprov_infer_time.get(query, (0, 0, 0))[0] for _, query in queries],
            width=bar_width,
            label="Infer",
        )
        ax.bar(
            [idx + group_offsets[4] for idx, _ in queries],
            [traceprov_forward_and_infer[query][0] for _, query in queries],
            width=bar_width,
            label="Trace + Infer",
        )
        ax.bar(
            [idx + group_offsets[5] for idx, _ in queries],
            [
                traceprov_forward_and_infer_and_material[query][0]
                for _, query in queries
            ],
            width=bar_width,
            label="Trace + Infer + Materialize",
        )
        plt.grid(axis="y", linestyle="--", alpha=0.7)
    else:
        group_width = 0.85
        num_groups = 4
        bar_width = group_width / num_groups
        group_offsets = np.linspace(
            -(num_groups - 1) / 2 * bar_width,
            (num_groups - 1) / 2 * bar_width,
            num_groups,
        )

        fig, (ax, ax_table) = plt.subplots(1, 2, figsize=(15, 6))

        ax.bar(
            [idx + group_offsets[0] for idx, _ in queries],
            [baseline_time[query][0] for _, query in queries],
            width=bar_width,
            label="Baseline",
        )

        # print(gprom_time)
        # for _, q in queries:
        #     print(len(gprom_time))
        #     print("searching for", q, q in gprom_time)

        # gprom_graph_data = [gprom_time.get(query, (0, 0))[0] for _, query in queries]
        # print(gprom_graph_data)
        ax.bar(
            [idx + group_offsets[1] for idx, _ in queries],
            [gprom_time.get(query, (0, 0))[0] for _, query in queries],
            width=bar_width,
            label="GProM (Join)",
        )
        ax.bar(
            [idx + group_offsets[2] for idx, _ in queries],
            [traceprov_forward_and_infer[query][0] for _, query in queries],
            width=bar_width,
            label="Trace + Infer",
        )
        ax.bar(
            [idx + group_offsets[3] for idx, _ in queries],
            [
                traceprov_forward_and_infer_and_material[query][0]
                for _, query in queries
            ],
            width=bar_width,
            label="Trace + Infer + Materialize",
        )

    ax.set_xticks(
        [idx for idx, _ in queries], labels=add_labels([q for _, q in queries]), size=8
    )

    ax.set_ylabel("Time (s)")
    ax.set_title(f"Execution and Provenance Measurement time ({label})")
    ax.set_xlabel("Query")
    ax.legend(loc="upper left", ncols=2, prop=dict(size=8))

    if len(gprom_time) > 0:
        ax.set_yscale("log", base=10)

    data = []
    for _, query in queries:
        record_tuple = traceprov_logged_records[query]
        record = [
            format(int(record_tuple[1]), ","),
            format(int(record_tuple[0]), ","),
            format(int(record_tuple[2]), ","),
        ]
        data.append(record)

    table = ax_table.table(
        cellText=np.asarray(data),
        rowLabels=add_labels([q for _, q in queries]),
        colLabels=[
            "Q1 of #records",
            "Q2 of #records",
            "Q3 of #records",
        ],
        colWidths=[0.2, 0.2, 0.2],
        cellLoc="center",
        loc="center",
    )
    ax_table.set_title("Number of records")

    table.auto_set_font_size(False)
    table.set_fontsize(10)
    table.scale(1.2, 1.5)
    ax_table.axis("off")
    ax_table.axis("tight")

    plt.tight_layout()
    plt.savefig(f"{out_dir}/execution_time_{label}.png")

    plt.clf()

    fig_2, ax_2 = plt.subplots()
    plt.grid(axis="x", linestyle="--", alpha=0.7)
    plt.grid(axis="y", linestyle="--", alpha=0.7)
    ax_2.set_xticks(
        [idx for idx, _ in queries], labels=add_labels([q for _, q in queries]), size=8
    )

    ax_2.set_xlabel("Query")
    ax_2.set_ylabel("Slowdown")

    def slowdown(traces, bases):
        if len(traces) != len(bases):
            print(
                "Got mismatching for slowdown: ",
                len(traces),
                len(bases),
                ": for: ",
                params,
            )
        return [(trace / base) for (trace, base) in zip(traces, bases)]

    gprom_slowdown = [
        (
            query,
            get_tuple(
                slowdown(forward_times(raw_gprom_time[query]), raw_baseline_time[query])
            ),
        )
        for _, query in queries
        if query in raw_gprom_time
    ]

    traceprov_forward_and_infer_slowdown = [
        (
            query,
            get_tuple(
                slowdown(
                    raw_traceprov_forward_and_infer[query], raw_baseline_time[query]
                )
            ),
        )
        for _, query in queries
    ]

    dump_traceprov_forward_and_infer_slowdown = {
        query: get_tuple(
            slowdown(raw_traceprov_forward_and_infer[query], raw_baseline_time[query])
        )
        for _, query in queries
    }

    traceprov_forward_and_infer_and_mat_slowdown = [
        (
            query,
            get_tuple(
                slowdown(
                    raw_traceprov_forward_and_infer_and_material[query],
                    raw_baseline_time[query],
                )
            ),
        )
        for _, query in queries
    ]

    dump_traceprov_forward_and_infer_and_mat_slowdown = {
        query: get_tuple(
            slowdown(
                raw_traceprov_forward_and_infer_and_material[query],
                raw_baseline_time[query],
            )
        )
        for _, query in queries
    }

    muller_slowdown = [
        (query, muller_results[query])
        for _, query in queries
        if query in muller_results
    ]

    # Now, we make the slowdown plot.
    plt.scatter(
        [
            reverse_query_mapping[query]
            for (query, _) in traceprov_forward_and_infer_slowdown
        ],
        [sd[0] for (_, sd) in traceprov_forward_and_infer_slowdown],
        label="Trace + Infer",
    )

    traceprov_forward_and_infer_err_high = [
        sd[2] - sd[0] for (_, sd) in traceprov_forward_and_infer_slowdown
    ]

    traceprov_forward_and_infer_err_low = [
        sd[0] - sd[1] for (_, sd) in traceprov_forward_and_infer_slowdown
    ]

    # plt.errorbar(
    #     [query.query for (query, _) in traceprov_forward_and_infer_slowdown],
    #     [sd[0] for (_, sd) in traceprov_forward_and_infer_slowdown],
    #     yerr=np.vstack(
    #         [traceprov_forward_and_infer_err_low, traceprov_forward_and_infer_err_high]
    #     ),
    #     fmt="o",
    # )
    plt.scatter(
        [
            reverse_query_mapping[query]
            for (query, _) in traceprov_forward_and_infer_and_mat_slowdown
        ],
        [sd[0] for (_, sd) in traceprov_forward_and_infer_and_mat_slowdown],
        label="Trace + Infer + Materialize",
    )

    traceprov_forward_and_infer_and_mat_err_high = [
        sd[2] - sd[0] for (_, sd) in traceprov_forward_and_infer_and_mat_slowdown
    ]

    traceprov_forward_and_infer_and_mat_err_low = [
        sd[0] - sd[1] for (_, sd) in traceprov_forward_and_infer_and_mat_slowdown
    ]

    # plt.errorbar(
    #     [
    #         reverse_query_mapping[query]
    #         for (query, _) in traceprov_forward_and_infer_and_mat_slowdown
    #     ],
    #     [sd[0] for (_, sd) in traceprov_forward_and_infer_and_mat_slowdown],
    #     yerr=np.vstack(
    #         [
    #             traceprov_forward_and_infer_and_mat_err_low,
    #             traceprov_forward_and_infer_and_mat_err_high,
    #         ]
    #     ),
    #     fmt="o",
    # )

    if len(gprom_slowdown):
        plt.scatter(
            [reverse_query_mapping[query] for (query, _) in gprom_slowdown],
            [sd[0] for (_, sd) in gprom_slowdown],
            label="GProM",
        )
        ax_2.set_yscale("log", base=10)

    if use_muller:
        plt.scatter(
            [reverse_query_mapping[query] for (query, _) in muller_slowdown],
            [sd for (_, sd) in muller_slowdown],
            label="Muller",
        )

    if use_muller:
        # Because muller's has a high slowdown
        ax_2.set_yscale("log", base=10)

    ax_2.legend(loc="lower center", prop=dict(size=8))
    ax_2.set_title(f"Slowdown ({label})")

    plt.savefig(f"{out_dir}/slowdown_{label}.png")

    assert_correct_type(dump_baseline_time)
    assert_correct_type(dump_forward_times)
    assert_correct_type(dump_infer_times)
    assert_correct_type(dump_traceprov_forward_and_infer)
    assert_correct_type(dump_traceprov_forward_and_infer_and_material)
    assert_correct_type(dump_traceprov_logged_records)
    assert_correct_type(dump_traceprov_forward_and_infer_slowdown)
    assert_correct_type(dump_traceprov_forward_and_infer_and_mat_slowdown)
    assert_correct_type(dump_gprom_time)
    assert_correct_type(dump_gprom_logged_records)

    return dict(
        baseline=dump_baseline_time,
        trace=dump_forward_times,
        infer=dump_infer_times,
        trace_and_infer=dump_traceprov_forward_and_infer,
        trace_and_infer_and_material=dump_traceprov_forward_and_infer_and_material,
        num_records=dump_traceprov_logged_records,
        trace_and_infer_slowdown=dump_traceprov_forward_and_infer_slowdown,
        trace_and_infer_and_materialize_slowdown=dump_traceprov_forward_and_infer_and_mat_slowdown,
        **(
            {}
            if not gprom_data
            else dict(
                gprom_join=dump_gprom_time,
                gprom_num_records=dump_gprom_logged_records,
            )
        ),
    )


def reorganize(content: Dict[str, Dict[str, Any]]):
    # current structure is {'header': {1: {}, 2: {}}}
    # Need to reorganize it to {1: {'header: ''}...}
    new_content = {}
    for header in content:
        queries_content = content[header]
        new_content = {
            **new_content,
            **{
                query: {**{header: query_content}, **new_content.get(query, {})}
                for query, query_content in queries_content.items()
            },
        }

    return new_content


def augment_contents(query_contents):
    median = {
        f"{header}_median": content[0] for header, content in query_contents.items()
    }
    variance = {
        f"{header}_variance": content[1] for header, content in query_contents.items()
    }
    return {**median, **variance}


def parse_back(instr: str):
    return instr.replace("_median", "").replace("_variance", "")


def dump_csv(contents_with_labels, file_prefix):

    dict_rows = [
        {
            "param": param,
            "query": query.add_label().label,
            **augment_contents(query_contents),
        }
        for param, param_contents in contents_with_labels.items()
        for query, query_contents in param_contents.items()
    ]

    order_header = [
        "param",
        "query",
        "baseline",
        "trace",
        "infer",
        "trace_and_infer",
        "trace_and_infer_and_material",
        "num_records",
        "trace_and_infer_slowdown",
        "trace_and_infer_and_materialize_slowdown",
        "gprom_join",
        "gprom_num_records",
    ]

    augmanted_order = []
    for header in order_header:
        if header in ["param", "query"]:
            augmanted_order.append(header)
            continue
        augmanted_order.append(f"{header}_median")
        augmanted_order.append(f"{header}_variance")

    rows = [
        [str(row.get(header, None)) for header in augmanted_order] for row in dict_rows
    ]

    rows = [augmanted_order, *rows]
    with open(f"{file_prefix}.tsv", "w") as f:
        tab_sep = ["\t".join(row) for row in rows]
        f.write("\n".join(tab_sep))


def main():
    baseline_files = []
    traceprov_files = []
    traceprov_files_infer = []
    gprom_files = []
    label_suff = None
    use_muller = False
    results_dir = "./"
    for idx, i in enumerate(sys.argv[1:], start=1):
        # if i == "-v":
        #     variance = True
        # if i == "-m":
        #     mean = True
        if i == "-base":
            baseline_files.append(sys.argv[idx + 1])
        if i == "-trace":
            traceprov_files.append(sys.argv[idx + 1])
        if i == "-trace_inf":
            traceprov_files_infer.append(sys.argv[idx + 1])
        if i == "-label":
            label_suff = sys.argv[idx + 1]
        if i == "-muller":
            use_muller = True
        if i == "-out":
            results_dir = sys.argv[idx + 1]
        if i == "-gprom":
            gprom_files.append(sys.argv[idx + 1])

    import os

    os.system(f"mkdir -p {results_dir}")

    params = [
        None,
        "params_default",
        "params_1",
        "params_2",
        "params_3",
        "params_4",
        "params_5",
    ]

    all_contents = {}
    for param in params:
        label = f"{label_suff}_{param or 'all'}"
        contents = generate_figures(
            baseline_files,
            traceprov_files,
            traceprov_files_infer,
            gprom_files,
            use_muller=use_muller,
            params=param,
            label=label,
            out_dir=results_dir,
        )
        new_param = param or "all"
        all_contents[new_param] = reorganize(contents)

    dump_csv(all_contents, f"{results_dir}/{label_suff}_dump")


if __name__ == "__main__":
    main()
