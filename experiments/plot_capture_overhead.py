###

from collections import defaultdict
from functools import reduce
from pathlib import Path
from typing import NamedTuple

from matplotlib import pyplot as plt
import numpy as np

from plot_overview import get_extra_predicates
from utils import (
    SYSTEM_COLORS,
    DataOptions,
    SystemLabels,
    get_options_split,
    make_category_data_mapped,
)
from traceprovpy.tools.file_utils import json_read_file, just_read
from traceprovpy.tools.plot_utils import (
    BenchmarkPlot,
    add_arrow_label,
    get_unique_handles_labels,
)
import duckdb

from sklearn.metrics import r2_score


class Reduced(NamedTuple):
    name: str
    cardinality: int
    children: list["Reduced"]
    extras: str

    def remove_projection(self):
        if self.name == "projection":
            return self.children[0].remove_projection()
        children_removed = tuple([child.remove_projection() for child in self.children])
        return self._replace(children=children_removed)

    def remove_all_extra(self):
        return self._replace(
            extras="",
            children=tuple([child.remove_all_extra() for child in self.children]),
        )

    def simple_check(self, other):
        return self.remove_all_extra() == other.remove_all_extra()

    def get_single_child(self):
        assert len(self.children) == 1
        return self.children[0]

    def make_prov_scan(self):
        if self.name == "seq_scan":
            return self._replace(name="prov_scan")
        if self.name == "column_data_scan":
            return self._replace(name="prov_scan")
        return self._replace(
            children=tuple([child.make_prov_scan() for child in self.children])
        )

    def get_row_width(self):
        if self.name in ("hash_group_by", "perfect_hash_group_by"):
            return 1
        if self.name == "ungrouped_aggregate":
            return 1
        if self.name == "prov_scan":
            return 1
        if self.name == "filter":
            return self.get_single_child().get_row_width()
        if self.name == "order_by":
            return self.get_single_child().get_row_width()
        if self.name == "hash_join":
            return self.children[0].get_row_width() + self.children[1].get_row_width()
        if self.name == "seq_scan":
            return 0
        if self.name == "column_data_scan":
            return 0
        if self.name == "delim_scan" or self.name == "dummy_scan":
            return 0

        if self.name == "streaming_limit":
            return self.get_single_child().get_row_width()
        if self.name == "left_delim_join":
            return 0
        assert False, f"Got unhandled {self.name}, {self}"

    def get_first_normal(self):
        if self.name in ("query", "result_collector"):
            return self.get_single_child().get_first_normal()
        return self

    def uses_perfect(self):
        extra_lower = self.extras.lower()
        if "build min" in extra_lower:
            return True
        return any(child.uses_perfect() for child in self.children)

    def get_row_width_diff(self, other: "Reduced", path: list[str]):
        assert self.name == other.name or other.name == "prov_scan"
        next_path = [*path, self.name]
        print("#######################")
        print(f"Diff at {next_path}", self.get_row_width(), other.get_row_width())
        # print("My extras ", self.extras)
        # print("Other extras ", other.extras)
        print("#######################")
        # print(other)
        assert len(self.children) == len(other.children)
        for idx, (self_child, other_child) in enumerate(
            zip(self.children, other.children, strict=True)
        ):
            new_path = [*next_path, f"child: {idx}"]
            self_child.get_row_width_diff(other_child, new_path)

    def get_total_cost(self, result: dict):
        if self.name == "hash_join":
            assert len(self.children) == 2
            left_total_width = self.children[0].get_row_width()
            right_total_width = self.children[1].get_row_width()
            result["left"] += left_total_width * self.cardinality
            result["right"] += right_total_width * self.cardinality

        if self.name == "order_by":
            result["order_by"] += self.children[0].get_row_width() * self.cardinality

        for child in self.children:
            child.get_total_cost(result)


def reduce_result(in_res: dict):
    return Reduced(
        name=in_res["name"].lower().strip(),
        cardinality=in_res["cardinality"],
        children=tuple(list(map(reduce_result, in_res["children"]))),
        extras=in_res.get("extra_info"),
    )


def assert_same(in_res: list[Reduced]):
    current = in_res[0]
    for res in in_res[1:]:
        assert current.simple_check(res)
    return current


class LogStats(NamedTuple):
    call_count: int = 0
    adjusted_size: int = 0


def _get_single_stats(capture_time: dict):
    def _reduce(prev: LogStats, curr):
        call_count = prev.call_count + curr["count"]
        adjusted_size = prev.adjusted_size + curr["sum"] * curr["column_count"]
        return LogStats(call_count=call_count, adjusted_size=adjusted_size)

    return reduce(_reduce, capture_time.values(), LogStats())


def get_layer_stats(capture_time: list[dict]):
    sep_stats = list(
        [
            _get_single_stats(result["option"]["misc_key_value_layer_stats"])
            for result in capture_time
        ]
    )
    current = sep_stats[0]
    for stat in sep_stats:
        if stat != current:
            print("Got diff call stats: ", stat, current)
    return current


# RECOGNIZED = ["left", "right", "order_by"]


LOG_FEATURES = [
    "call_count",
    "adjusted_size",
]

PROPAGATE_FEATURES = [
    "left",
    "right",
    "order_by",
]

FEATURES = [*LOG_FEATURES, *PROPAGATE_FEATURES]

SELECTED_FEATURES = [
    "call_count",
    "adjusted_size",
    "left",
    "right",
    "order_by",
]


def reorder(in_dict, features):
    return [
        value
        # the sort should always happen
        for (key, value) in sorted(
            in_dict.items(), key=lambda x: SELECTED_FEATURES.index(x[0])
        )
        if key in features
    ]


def get_adjusted_r2(correct, pred, X_orig):
    n_samples = X_orig.shape[0]
    n_feat = X_orig.shape[1]
    r2 = r2_score(correct, pred)
    adjusted_r2 = 1 - ((1 - r2) * (n_samples - 1) / (n_samples - n_feat - 1))
    return adjusted_r2


def get_traceprov_results(in_json: Path):
    tp_file = json_read_file(in_json)
    all_results = tp_file["results"]
    query_profiles = {
        q: dict(
            base=assert_same(list(map(reduce_result, qdata["result"]["base_profile"])))
            .remove_projection()
            .get_first_normal(),
            capture=assert_same(
                list(map(reduce_result, qdata["result"]["capture_profile"]))
            )
            .remove_projection()
            .get_first_normal(),
            capture_total_sum=get_layer_stats(qdata["result"]["capture_time"]),
        )
        for q, qdata in all_results.items()
    }
    same_queries = {}
    for query, query_result in query_profiles.items():
        if query_result["base"].simple_check(query_result["capture"]):
            print("Plan Same: ", query, query_result["capture"])
            same_queries[query] = query_result
        else:
            # print(query_result["base"])
            # print(query_result["capture"])
            print("Plan diff: ", query)
    return same_queries, query_profiles


def plot_predictions(in_json: Path, traceprov_overhead: dict, out_dir: Path):
    same_queries, query_profiles = get_traceprov_results(in_json)
    log_based_predictions = make_prediction(
        same_queries, query_profiles, traceprov_overhead, LOG_FEATURES
    )
    propagate_based_predictions = make_prediction(
        same_queries, query_profiles, traceprov_overhead, PROPAGATE_FEATURES
    )
    all_features_based_predictions = make_prediction(
        same_queries, query_profiles, traceprov_overhead, FEATURES
    )
    true_overhead_pack = log_based_predictions["true_overhead"]
    overhead_fig, (overhead_axis) = plt.subplots(
        1,
        1,
        figsize=(4, 2),
    )
    x_axis_values = [item[0] for item in true_overhead_pack]
    true_overhead = list([item[1][-1] for item in true_overhead_pack])
    true_overhead_sorted = list(
        [idx for (idx, val) in sorted(enumerate(true_overhead), key=lambda x: x[1])]
    )
    x_axis = np.arange(len(x_axis_values))

    def _apply_sort(in_value):
        return [in_value[tos] for tos in true_overhead_sorted]

    # traceprov_name = SYSTEM_LABELS
    overhead_axis.plot(
        (x_axis),
        _apply_sort(true_overhead),
        label="True",
        marker=".",
    )
    overhead_axis.plot(
        (x_axis),
        _apply_sort(log_based_predictions["predicted"]),
        linestyle="dashed",
        label="Log",
        # marker=".",
    )
    # overhead_axis.plot(
    #     (x_axis),
    #     _apply_sort(propagate_based_predictions["predicted"]),
    #     linestyle="dashdot",
    #     label="Prop.",
    #     # marker=".",
    # )
    overhead_axis.plot(
        (x_axis),
        _apply_sort(all_features_based_predictions["predicted"]),
        linestyle="dotted",
        label="Log & Prop.",
        color="red",
        linewidth=2.5,
        # marker=".",
    )
    legend_labels_all, handles_all = get_unique_handles_labels(overhead_axis)
    log_r2, log_r2_adj = (
        str(round(log_based_predictions["r2"], 2)),
        str(round(log_based_predictions["adjusted_r2"], 2)),
    )
    log_prop_r2, log_prop_r2_adj = (
        str(round(all_features_based_predictions["r2"], 2)),
        str(round(all_features_based_predictions["adjusted_r2"], 2)),
    )

    def make_legend_label(norm_label, r2, r2_adjusted):
        return rf"{norm_label} (adj. $R^2:{r2_adjusted}$)"

    legend_labels_all[1] = make_legend_label(legend_labels_all[1], log_r2, log_r2_adj)
    legend_labels_all[2] = make_legend_label(
        legend_labels_all[2], log_prop_r2, log_prop_r2_adj
    )
    overhead_axis.legend(
        handles=handles_all,
        labels=legend_labels_all,
        prop=dict(size=7),
    )
    overhead_axis.set_xlabel(r"Query (absolute overhead $\longrightarrow$)")
    overhead_axis.set_xticks((x_axis), _apply_sort(x_axis_values))
    add_arrow_label(overhead_axis, "Abs. Overhead (s)", 0.1)
    overhead_fig.savefig(out_dir / "overhead_prediction.pdf", bbox_inches="tight")
    # propagate_based_predictions


def make_prediction(same_queries, query_profiles, traceprov_overhead, features):

    cost_overhead = {}
    for query, query_result in same_queries.items():
        # if not query in ["3", "5", "8", "10"]:
        #     continue
        print("################################")
        print("Query: ", query)
        base = query_result["base"]
        other: Reduced = query_result["capture"].make_prov_scan()
        # base.get_row_width_diff(other, [])
        total_cost = dict(left=0.0, right=0.0, order_by=0.0)
        other.get_total_cost(total_cost)
        cts = query_profiles[query]["capture_total_sum"]
        print("Total Cost: ", total_cost, query_profiles[query]["capture_total_sum"])
        cost_overhead[query] = np.array(
            [
                *reorder(
                    {
                        **total_cost,
                        "call_count": cts.call_count,
                        "adjusted_size": cts.adjusted_size,
                    },
                    features,
                ),
                traceprov_overhead[query],
            ]
        )
        print("################################")
        # print("Width: ",)
    cost_overhead_ordered = [
        co for co in sorted(cost_overhead.items(), key=lambda x: int(x[0]))
    ]
    cost_overhead_ordered_values = np.array([co[1] for co in cost_overhead_ordered])
    # left_cost = np.array([coo_obj[1][0][0] for coo_obj in cost_overhead_ordered])
    # right_cost = np.array([coo_obj[1][0][1] for coo_obj in cost_overhead_ordered])
    # call_cost = np.array([coo_obj[1][0][2] for coo_obj in cost_overhead_ordered])
    # adjusted_size_cost = np.array(
    #     [coo_obj[1][0][3] for coo_obj in cost_overhead_ordered]
    # )
    # actual_cost = np.array([coo_obj[1][1] for coo_obj in cost_overhead_ordered])
    # print(cost_overhead)
    # stack = (left_cost, right_cost, call_cost, adjusted_size_cost)
    # # stack = (call_cost, adjusted_size_cost)
    # print(stack)
    X = cost_overhead_ordered_values[:, :-1]
    actual_cost = np.array(cost_overhead_ordered_values[:, -1])
    print(X, actual_cost)
    params = np.linalg.lstsq(X, actual_cost, rcond=None)[0]
    param_arr = np.array(params)
    print("FEATERE PARAM", features, params)
    # for _idx, value in enumerate(
    #     zip(left_cost, right_cost, call_cost, adjusted_size_cost, actual_cost)
    #     # zip(call_cost, adjusted_size_cost, actual_cost)
    # ):
    #     param_input = np.array(value[:-1])
    #     result = np.dot(param_input, param_arr)
    #     coo = cost_overhead_ordered[_idx]
    #     qp = query_profiles[coo[0]]
    #     print(
    #         cost_overhead_ordered[_idx][0],
    #         result,
    #         value[-1],
    #         qp["capture"].uses_perfect(),
    #     )

    z_pred = X @ np.array(params)
    for idx, value in enumerate(z_pred):
        print(cost_overhead_ordered[idx][0], value, cost_overhead_ordered[idx][-1][-1])
    # print(z_pred)
    ss_res = np.sum((actual_cost - z_pred) ** 2)
    ss_tot = np.sum((actual_cost - actual_cost.mean()) ** 2)
    r2 = 1 - ss_res / ss_tot
    print("Feature: ", features)
    print(f"R2 (manual) = {r2:.4f}")
    print(f"{list(actual_cost)}, {list(z_pred)}")
    r2_lib = r2_score(list(actual_cost), list(z_pred))
    print(f"R2 (lib) = {r2_lib:.4f}")
    adjusted_r2 = get_adjusted_r2(actual_cost, z_pred, X)
    print(f"Adjusted R2: ", adjusted_r2)
    return dict(
        r2=r2_lib,
        adjusted_r2=adjusted_r2,
        true_overhead=cost_overhead_ordered,
        predicted=z_pred,
    )


def main():
    plot_context = BenchmarkPlot("plot_capture_overhead", ignore_args=True)
    plot_context.parser.add_argument("--db_dir", required=True)
    plot_context.parser.add_argument("--sf", required=True, type=int)
    plot_context.parser.add_argument("--traceprov_result", required=True)
    plot_context.parser.add_argument(
        "--postgres_tpch_dir", required=False, default=("../postgres/benchmark/tpch/")
    )
    plot_context.parser.add_argument(
        "--duckdb_tpch_dir", required=False, default=("../duckdb/benchmark/tpch/")
    )
    parsed = plot_context.parser.parse_args()

    db_option = lambda _mode: DataOptions(
        sf=f"sf_{parsed.sf}", db_name="duckdb", mode=_mode
    )
    all_db_path = Path(parsed.db_dir) / db_option("all").to_db()
    offset_db_path = Path(parsed.db_dir) / db_option("offset").to_db()
    parsed.db = offset_db_path.name
    out_dir = plot_context.add_timestamp()
    assert all_db_path.exists(), f"expected {all_db_path} to exist!"
    assert offset_db_path.exists(), f"expected {offset_db_path} to exist!"

    conn = duckdb.connect()
    conn.execute(f"attach '{all_db_path.as_posix()}' as data_all")
    conn.execute(f"attach '{offset_db_path.as_posix()}' as data_offset")
    cursor = conn.cursor()
    sd_comparison_query = just_read("./sd_comparison_query.sql")
    cursor.execute(sd_comparison_query)
    results = list(cursor.fetchall())
    result_map = {
        cell[0]: {
            q: qres["phase_1_absolute_overhead"]
            for q, qres in make_category_data_mapped(cell[1]).items()
        }
        for cell in results
    }
    traceprov_overhead = result_map["traceprov"]
    plot_predictions(parsed.traceprov_result, traceprov_overhead, out_dir)


if __name__ == "__main__":
    main()
