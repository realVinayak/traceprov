import glob
import json
import os
from pathlib import Path

from traceprovpy.tools.file_utils import get_tmp_file, json_read_file, just_write
from traceprovpy.tools.normalized_row import (
    NormalizedSampleInferRow,
    extract_bucket_category,
    extract_stats_sample_infer_row,
    extract_traceprov,
    category_order,
    plot_partition_result,
)
from traceprovpy.tools.plot_utils import BenchmarkPlot
import duckdb

import matplotlib.pyplot as plt
import numpy as np

from traceprovpy.utils import add_underscores_numbers

class NormalizedRowSelctivity(NormalizedSampleInferRow):
    num_rows: int
    selectivity: int

    def keys(self):
        return super().keys() | {"num_rows", "selectivity"}


def plot_result_for_num(result: dict, out_dir: Path):
    num_rows = result["num_rows"]
    x_axis_value = [1, 5, 10, 50, 100, 500, 1000, 5000, 8500, 9500]

    remap_data = {
        el["category"]: {el_child["selectivity"]: el_child for el_child in el["data"]}
        for el in result["data"]
    }
    width = 0.1
    new_width = 0.12
    fig_title = f"Number of rows: {add_underscores_numbers(int(num_rows))}"
    plot_partition_result(
        x_axis_value,
        [str(x / 100) for x in x_axis_value],
        out_dir,
        remap_data,
        "Selectivity",
        fig_title,
        fig_title,
        width,
        new_width,
        num_rows,
    )


def main():
    parser = BenchmarkPlot("analyze_partition")
    parsed = parser.parser.parse_args()
    all_results = []
    for file in parsed.files:
        complete_path = f"{parsed.root}/{file}/result.json"
        paths = glob.glob(complete_path)
        for path in paths:
            result: dict = json_read_file(path)
            dir_name = Path(path).parts[-2]
            print(dir_name)
            bucket = extract_bucket_category(dir_name)
            all_results.append((path, bucket, result["result"]))
            # result["result"]
            print(len(result["result"]))

    all_normalized = []
    for outer_row in all_results:
        category = f"TraceProv(bucket: {outer_row[1]})"
        for row in outer_row[2]:
            main_result = row["result"]
            traceprov = main_result["traceprov"]
            sample_infer = main_result["sample_inference"]
            row_args = dict(
                num_rows=int(row["dir"]),
                selectivity=int(row["selectivity"]),
                category=category,
                **extract_traceprov(traceprov),
                **extract_stats_sample_infer_row(
                    sample_infer["sql_spec_map"][1:], sample_infer["result_time"][1:]
                ),
            )
            norm_row = NormalizedRowSelctivity(**row_args)
            normed = norm_row.normalize()
            all_normalized.extend(normed)

    conn = duckdb.connect("test.db")
    cursor = conn.cursor()
    just_write("normalized.json", json.dumps(all_normalized))
    cursor.execute(
        "create or replace table dumped as (select * from read_json_auto('normalized.json'))"
    )

    sql_query = """
    select struct_pack(num_rows := num_rows, data := list(data)) from (
        select 
            num_rows, 
            struct_pack(category := category, data := list(data)) as data from (
                select category, num_rows, struct_pack(selectivity := selectivity, base_median := median(base_time), capture_median := median(capture_time), base_variance := stddev(base_time) / mean(base_time), capture_variance := stddev(capture_time) / mean(capture_time), infer_time := ANY_VALUE(average_time), infer_variance := ANY_VALUE(max_stdev_ratio) ) as data from dumped where iter >= 6 group by category, num_rows, selectivity
                ) group by num_rows, category
    ) group by num_rows;
    """
    cursor.execute(sql_query)
    fetched_result = cursor.fetchall()
    out_dir = Path(get_tmp_file())
    os.makedirs(out_dir, exist_ok=True)
    for fetch_row in fetched_result:
        print(fetch_row[0])
        plot_result_for_num(fetch_row[0], out_dir)


if __name__ == "__main__":
    main()
