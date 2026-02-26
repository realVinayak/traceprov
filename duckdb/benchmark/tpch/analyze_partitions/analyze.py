import glob
import json
import os
from pathlib import Path

from traceprovpy.tools.file_utils import json_read_file, just_write
from traceprovpy.tools.normalized_row import (
    NormalizedSampleInferRow,
    extract_bucket_category,
    extract_stats_sample_infer_row,
    extract_traceprov,
    plot_partition_result,
)
from traceprovpy.tools.plot_utils import BenchmarkPlot
import duckdb

import matplotlib.pyplot as plt
import numpy as np


class NormalizedRowTpch(NormalizedSampleInferRow):
    query_num: str

    def keys(self):
        return super().keys() | {"query_num"}


def plot_result_for_num(result: list[dict], out_dir: Path, sf):
    x_axis_value = list(map(str, [3, 4, 5, 7, 9, 10, 11, 12, 15, 18, 21]))
    remap_data = {
        el["category"]: {el_child["query_num"]: el_child for el_child in el["data"]}
        for el in result
    }
    fig_title = f"Scale Factor: {sf}"
    plot_partition_result(
        x_axis_value,
        x_axis_value,
        out_dir,
        remap_data,
        "Query",
        fig_title,
        fig_title,
        0.1,
        0.12,
        str(sf)
    )


def main():
    parser = BenchmarkPlot("analyze_partition")
    parser.parser.add_argument("-sf", required=True)
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
            all_results.append((path, bucket, result))

    out_dir = Path(f"./tmp/{parsed.sf}")
    os.makedirs(out_dir, exist_ok=True)
    all_normalized = []
    for outer_row in all_results:
        category = f"TraceProv(bucket: {outer_row[1]})"
        for query_num, query_result in outer_row[2].items():
            sample_infer = query_result["sample_inference_result"]
            row_args = dict(
                query_num=str(query_num),
                category=category,
                **extract_traceprov(query_result["result"]),
                **extract_stats_sample_infer_row(
                    sample_infer["sql_spec_map"][1:], sample_infer["result_time"][1:]
                ),
            )
            norm_row = NormalizedRowTpch(**row_args)
            normed = norm_row.normalize()
            all_normalized.extend(normed)

    conn = duckdb.connect(out_dir / "test.db")
    cursor = conn.cursor()
    path = just_write(out_dir / "normalized.json", json.dumps(all_normalized))
    if isinstance(path, Path):
        path = path.as_posix()

    cursor.execute(
        f"create or replace table dumped as (select * from read_json_auto('{path}'))"
    )

    sql_query = """
    select
    struct_pack(category := category, data := list(data)) as data from (
        select category, struct_pack(query_num := query_num, base_median := median(base_time), capture_median := median(capture_time), base_variance := stddev(base_time) / mean(base_time), capture_variance := stddev(capture_time) / mean(capture_time), infer_time := ANY_VALUE(average_time), infer_variance := ANY_VALUE(max_stdev_ratio) ) as data from dumped where iter >= 6 group by category, query_num
        ) group by category
    """
    cursor.execute(sql_query)
    fetched_result = cursor.fetchall()
    print(fetched_result[0])
    plot_result_for_num(
        [fetched_result[i][0] for i in range(len(fetched_result))], out_dir, parsed.sf
    )


if __name__ == "__main__":
    main()
