# the simpler handling case.

import argparse
import json
import os
from pathlib import Path
from typing import Tuple

from matplotlib import pyplot as plt
import matplotlib as mpl
import numpy as np

from traceprovpy.tools.duckdb_inference import DuckDBDriverOptions
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse
from traceprovpy.tools.file_utils import (
    json_read_file,
    just_read,
    just_write,
    null_safe,
)
from traceprovpy.tools.normalized_row import (
    Extendable,
    Normalizable,
    NormalizedRow,
    extract_infer,
    make_dummy_profile_result,
    make_dummy_simple_result,
    tap_profile_result,
    tap_simple_result,
)
from traceprovpy.tools.plot_utils import BenchmarkPlot
from traceprovpy.tools.run_duckdb_generic import add_query_options
import duckdb

mpl.rcParams.update(
    {
        # fonts
        "font.family": "serif",
        "font.size": 16,
        "axes.labelsize": 18,
        "xtick.labelsize": 16,
        "ytick.labelsize": 16,
        "legend.fontsize": 15,
        "axes.titlesize": 18,
        # cleaner look
        "axes.spines.top": False,
        "axes.spines.right": False,
        # lines
        "lines.linewidth": 2,
        "patch.linewidth": 1.5,
    }
)


class NormalizedDuckTPCHRow(Normalizable):
    category: str
    query_num: str
    phase_1: Extendable
    phase_1_profile: Extendable
    phase_2: Extendable

    def keys(self):
        return {
            "category",
            "query_num",
            "phase_1",
            "phase_1_profile",
            "phase_2",
        }


ERROR_CODES = {"errorcode", "timeout"}


class ResultAnalyzer:

    def __init__(self):
        self.added_base = False

    def gprom(self, key: str, path: Path) -> list[NormalizedDuckTPCHRow]:
        print("Handling GProM!", key)
        result = json_read_file(path, True)
        query_results = result["results"]
        rows: list[NormalizedDuckTPCHRow] = []
        for query_num, query_data in query_results.items():
            for category, category_data in query_data.items():
                if "timeout" in category_data:
                    print("Handing timeout!")
                    rows.append(
                        NormalizedDuckTPCHRow(
                            category=category,
                            query_num=query_num,
                            phase_1=Extendable([make_dummy_simple_result(-1)] * 15),
                            phase_1_profile=Extendable(
                                [make_dummy_profile_result(-1)] * 15
                            ),
                            phase_2=None,
                        )
                    )
                    continue
                elif "errorcode" in category_data:
                    rows.append(
                        NormalizedDuckTPCHRow(
                            category=category,
                            query_num=query_num,
                            phase_1=Extendable([make_dummy_simple_result(-2)] * 15),
                            phase_1_profile=Extendable(
                                [make_dummy_profile_result(-2)] * 15
                            ),
                            phase_2=None,
                        )
                    )
                    continue
                base = list(map(tap_simple_result, category_data["time"]))
                base_profile = list(map(tap_profile_result, category_data["profile"]))
                rows.append(
                    NormalizedDuckTPCHRow(
                        category=category,
                        query_num=query_num,
                        phase_1=Extendable(base),
                        phase_1_profile=Extendable(base_profile),
                        phase_2=None,
                    )
                )
        print("Len rows: ", len(rows))
        return rows

    def traceprov(
        self,
        key: str,
        path: Path,
    ) -> list[NormalizedDuckTPCHRow]:
        print("Handling TraceProv!", key)
        result = json_read_file(path, True)
        query_results = result["results"]
        rows: list[NormalizedDuckTPCHRow] = []
        base_rows: list[NormalizedDuckTPCHRow] = []
        bench_parser = make_duckdb_parse()
        add_query_options(bench_parser)
        call_options = result["call_options"]
        bench_parsed, _ = bench_parser.parse_known_args(call_options)
        for query_num, query_data in query_results.items():
            query_result = query_data["result"]
            base = list(map(tap_simple_result, query_result["base_time"]))
            base_profile = list(map(tap_profile_result, query_result["base_profile"]))
            base_rows.append(
                NormalizedDuckTPCHRow(
                    category="base",
                    query_num=query_num,
                    phase_1=Extendable(base),
                    phase_1_profile=Extendable(base_profile),
                    phase_2=None,
                )
            )
            phase_1 = list(map(tap_simple_result, query_result["capture_time"]))
            phase_1_profile = list(
                map(tap_profile_result, query_result["capture_profile"])
            )
            phase_2 = extract_infer(query_result["infer_results"])
            rows.append(
                NormalizedDuckTPCHRow(
                    category=DuckDBDriverOptions.get_suffix(bench_parsed),
                    query_num=query_num,
                    phase_1=Extendable(phase_1),
                    phase_1_profile=Extendable(phase_1_profile),
                    phase_2=Extendable(phase_2),
                )
            )
        all_rows = [*rows]
        if not self.added_base:
            all_rows.extend(base_rows)
        self.added_base = True
        return all_rows


def plot_data(
    data: list[Tuple[str, dict]],
    categories: list[str],
    label: str,
    sf: str,
    out_dir: Path,
    width: float,
    group_gap: float,
    category_label_mapping: dict = None,
    fig_size=(14, 3),
):
    x_axis_values = list(map(str, range(1, 23)))
    x_axis = np.arange(len(x_axis_values))
    slowdown_fig, slowdown_axis = plt.subplots(1, 1, figsize=fig_size)
    data = [item for item in data if item[0] in categories]
    result_sorted = sorted(data, key=lambda x: categories.index(x[0]))
    pending_bars = []
    for category_idx, (category, catagory_data) in enumerate(result_sorted):
        x_axis_adjusted = (
            x_axis * (len(categories) * width + group_gap) + width * category_idx
        )

        def _get_key_in_dict(in_key: str):
            return null_safe(
                [
                    catagory_data[q][in_key] if q in catagory_data else 0
                    for q in x_axis_values
                ]
            )

        phase_all_slowdown = _get_key_in_dict("phase_all_slowdown")
        y_values = _get_key_in_dict("phase_1_explain_time_mean")
        y_error_values = [
            y_idx for y_idx, y in enumerate(y_values) if -2.5 < y and y < -1.5
        ]
        y_timeout_values = [
            y_idx for y_idx, y in enumerate(y_values) if -1.5 < y and y < -0.5
        ]
        print(y_error_values, y_timeout_values)
        print(y_values)
        phase_all_slowdown = [
            0 if (idx in [*y_error_values, *y_timeout_values]) else val
            for (idx, val) in enumerate(phase_all_slowdown)
        ]

        slowdown_axis.bar(
            x_axis_adjusted,
            phase_all_slowdown,
            width=width,
            label=(category_label_mapping or dict()).get(category, category),
            color=BenchmarkPlot.colors[category_idx],
        )
        # if y_error_values:
        #     slowdown_axis.bar(
        #         [x_axis_adjusted[idx] for idx in y_error_values],
        #         10,
        #         width=width,
        #         label="Error",
        #         color="lightgray",
        #         hatch="///",
        #         edgecolor="blue",
        #     )
        if y_timeout_values:
            pending_bars.append(
                dict(
                    x=[x_axis_adjusted[idx] for idx in y_timeout_values],
                    height=10**5,
                    width=width,
                    label="Timeout (> 5 min)",
                    color="lightgray",
                    hatch="///",
                    edgecolor="red",
                )
            )

    for bar in pending_bars:
        slowdown_axis.bar(**bar)
    slowdown_axis.legend(loc="upper left", bbox_to_anchor=(0, 1.2))

    slowdown_axis.set_ylim(bottom=0.5, top=300)
    slowdown_axis.annotate(
        "~525x",
        xy=(
            x_axis_values.index("11") * (len(categories) * width + group_gap) + 3.5,
            50,
        ),
        ha="center",
        fontsize=14,
        color="red",
        fontweight="bold",
    )
    slowdown_axis.annotate(
        "~1060x",
        xy=(
            x_axis_values.index("22") * (len(categories) * width + group_gap) + 3,
            50,
        ),
        ha="center",
        fontsize=14,
        color="red",
        fontweight="bold",
    )

    slowdown_axis.axhline(y=1, color="r", linestyle="--")
    slowdown_axis.axhline(y=2, color="r", linestyle="--")
    slowdown_axis.set_ylabel("Slowdown")
    slowdown_axis.set_xlabel("Query")
    slowdown_axis.set_yscale("log")
    slowdown_axis.set_xticks(
        x_axis * (len(categories) * width + group_gap) + width * (len(categories) / 3),
        x_axis_values,
    )
    slowdown_fig.suptitle(f"DuckDB Slowdown for SF={sf}")
    slowdown_fig.savefig(out_dir / f"{label}_slowdown.pdf", bbox_inches="tight")


def gen_plots(database_file: Path, out_dir: Path, sf):
    con = duckdb.connect(database_file)
    cursor = con.cursor()
    query = """
    SELECT category, list(normalized_slowdown) AS rows
    FROM normalized_slowdown
    GROUP BY category;
    """
    cursor.execute(query)
    results = cursor.fetchall()
    cursor.close()
    con.close()
    result_mapped = [
        (key, {query_data["query_num"]: query_data for query_data in data})
        for (key, data) in results
    ]
    all_categories = [
        "gprom_join",
        "gprom_window",
        "gprom_join_heuristics",
        "gprom_window_heuristics",
        "optimized-y__threads-1__compact-y__merge_chunks-y",
        "optimized-y__threads-1__compact-y__merge_chunks-y__table_stats-y",
    ]
    mapping = {
        "optimized-y__threads-1__compact-y__merge_chunks-y": "TraceProv",
        "optimized-y__threads-1__compact-y__merge_chunks-y__table_stats-y": "TraceProv (Stats.)",
        "gprom_join_heuristics": "GProM Join Heu.",
        "gprom_window_heuristics": "GProM Win. Heu.",
    }
    interesting_categories = [
        "gprom_join_heuristics",
        "gprom_window_heuristics",
        "optimized-y__threads-1__compact-y__merge_chunks-y__table_stats-y",
    ]
    plot_data(
        result_mapped,
        all_categories,
        "all",
        sf,
        out_dir,
        0.2,
        0.4,
        mapping,
        fig_size=(20, 3),
    )
    mapping["optimized-y__threads-1__compact-y__merge_chunks-y__table_stats-y"] = (
        "TraceProv"
    )
    plot_data(
        result_mapped,
        interesting_categories,
        "duckdb_interesting",
        sf,
        out_dir,
        0.7,
        0.8,
        mapping,
    )


def main():
    parser = argparse.ArgumentParser("tpch_analyzer")
    parser.add_argument("--dir", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--out_dir", required=True)
    parser.add_argument("--sf", required=True)
    parsed = parser.parse_args()
    config = json_read_file(parsed.config, True)
    out_dir = Path(parsed.out_dir)
    os.makedirs(out_dir, exist_ok=True)
    assert config is not None
    in_dir = Path(parsed.dir)
    normalized_rows: list[NormalizedDuckTPCHRow] = []
    analyze = ResultAnalyzer()
    for key, item in config.items():
        for file in item["file"]:
            result_file_path: Path = in_dir / file / "result.json"
            assert result_file_path.exists(), f"Expected {result_file_path} to exist!"
            _rows = getattr(analyze, item["handler"])(key, result_file_path)
            normalized_rows.extend(_rows)
    flatted = [row for group in normalized_rows for row in group.normalize()]
    path = just_write(out_dir / "flat.json", json.dumps(flatted))
    if isinstance(path, Path):
        path = path.as_posix()
    db_file = out_dir / "analyze.db"
    conn = duckdb.connect(db_file)
    cursor = conn.cursor()
    cursor.execute(
        f"create or replace table normalized as (select * from read_json_auto('{path}'))"
    )
    stats_sql = just_read("./query_stats.sql")
    cursor.execute(stats_sql)
    slowdown_sql = just_read("./query_slowdown.sql")
    cursor.execute(slowdown_sql)
    cursor.close()
    conn.close()
    gen_plots(db_file, out_dir, parsed.sf)


if __name__ == "__main__":
    main()
