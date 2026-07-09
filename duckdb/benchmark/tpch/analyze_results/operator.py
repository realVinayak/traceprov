import argparse
from pathlib import Path

from traceprovpy.tools.file_utils import json_read_file, traceprov_assert_safe_run
import duckdb


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", required=True)
    parser.add_argument("--dir", required=True)
    parser.add_argument("--root", required=True)
    parser.add_argument("--sf", required=True)
    parsed = parser.parse_args()
    config = json_read_file(parsed.config)
    out_dir: Path = Path(parsed.dir) / parsed.sf
    db = out_dir / "test.db" 
    normal_file = Path(parsed.root) / config['normal'] / "result.json"
    order_file = Path(parsed.root) / config['order'] / "result.json"
    normal_out_file = out_dir / "normal.json"
    traceprov_assert_safe_run(f"node analyze_operator_timings.mjs {normal_file.as_posix()} {normal_out_file.as_posix()}")
    order_out_file = out_dir / "order.json"
    traceprov_assert_safe_run(f"node analyze_operator_timings.mjs {order_file.as_posix()} {order_out_file.as_posix()}")

    conn = duckdb.connect(db)
    cursor = conn.cursor()
    cursor.execute(
        f"create or replace table smokedduck_normal as (select * from read_json_auto('{normal_out_file.as_posix()}'))"
    )
    cursor.execute(
        f"create or replace table smokedduck_order as (select * from read_json_auto('{order_out_file.as_posix()}'))"
    )

    cursor.close()
    conn.close()


    # traceprov_assert_safe_run(")


if __name__ == '__main__':
    main()