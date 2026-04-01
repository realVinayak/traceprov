import argparse
import os
from pathlib import Path
from traceprovpy.tools.benchmark_utils import traceprov_dump_safe_results
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse
from traceprovpy.tools.run_duckdb_generic import (
    infer_sample_id,
    json_read_file,
    just_read,
    just_write,
    run_combined,
    run_sample_inference,
    run_sample_inference_smokedduck,
    run_single,
    run_single_smokedduck,
)

import random

# to make sampling reproducible.
random.seed(10)


def get_filter_group(num_groups, selectivity, mode):
    multiplier = -1 if mode == "pre" else 1
    print(num_groups * selectivity, "num_gs")
    return int(selectivity * num_groups / 100) * multiplier


def run():
    base_parser = make_duckdb_parse()
    base_parser.add_argument("--num_groups", required=True, type=int)
    base_parser.add_argument("--config", required=True)
    base_parser.add_argument(
        "--random", action=argparse.BooleanOptionalAction, default=True
    )
    base_parser.add_argument("--mode", choices=["pre", "post"], default="post")
    parsed = base_parser.parse_args()

    result = []
    tmp = Path("./tmp/")
    os.makedirs(tmp, exist_ok=True)
    total_iters = 10
    base_sql = just_read(Path(f"queries/{parsed.mode}_base.sql"))
    base_offset_sql = just_read(Path(f"queries/{parsed.mode}_base_offset.sql"))
    capture_sql = just_read(Path(f"queries/{parsed.mode}_capture.sql"))
    capture_new_sql = just_read(Path(f"queries/{parsed.mode}_capture_new.sql"))
    capture_new_compact_sql = just_read(
        Path(f"queries/{parsed.mode}_capture_new_compact.sql")
    )
    validate_sql = just_read(Path(f"queries/{parsed.mode}_validate.sql"))
    validate_new_sql = just_read(Path(f"queries/{parsed.mode}_validate_new.sql"))
    validate_offset_sql = just_read(Path(f"queries/{parsed.mode}_validate_offset.sql"))
    validate_new_offset_sql = just_read(
        Path(f"queries/{parsed.mode}_validate_new_offset.sql")
    )
    validate_sd_sql = just_read(Path(f"queries/{parsed.mode}_validate_sd.sql"))
    validate_sd_new_sql = just_read(Path(f"queries/{parsed.mode}_validate_new_sd.sql"))
    config = json_read_file(parsed.config)
    assert config is not None

    query = "query"
    os.makedirs(tmp / query, exist_ok=True)
    parsed.root = tmp.as_posix()
    parsed.base_root = tmp.as_posix()

    for query_dir in config:

        def run(raw_selectivity):
            selectivity = get_filter_group(
                parsed.num_groups, raw_selectivity, parsed.mode
            )

            def replacer(in_sql: str):
                query_str = in_sql.replace("ROW_COUNT", str(query_dir["num_rows"]))
                query_str = query_str.replace(":selectivity", str(selectivity))
                if not parsed.random:
                    query_str = query_str.replace("_random", "")
                return query_str

            just_write(tmp / query / "base.sql", replacer(base_sql))
            just_write(tmp / query / "base_offset.sql", replacer(base_offset_sql))
            just_write(tmp / query / "capture.sql", replacer(capture_sql))
            just_write(tmp / query / "capture_new.sql", replacer(capture_new_sql))
            just_write(
                tmp / query / "capture_new_compact.sql",
                replacer(capture_new_compact_sql),
            )
            just_write(tmp / query / "validate.sql", replacer(validate_sql))
            just_write(tmp / query / "validate_sd.sql", replacer(validate_sd_sql))
            just_write(
                tmp / query / "validate_offset.sql", replacer(validate_offset_sql)
            )
            just_write(
                tmp / query / "validate_new_offset.sql",
                replacer(validate_new_offset_sql),
            )
            just_write(
                tmp / query / "validate_new_sd.sql", replacer(validate_sd_new_sql)
            )
            just_write(tmp / query / "validate_new.sql", replacer(validate_new_sql))

            query_result = run_combined(parsed, total_iters, query, [])
            result.append(
                dict(
                    dir=query_dir["num_rows"],
                    selectivity=selectivity,
                    result=query_result,
                    optimized=parsed.optimized,
                )
            )

        for sel in query_dir["sel"]:
            run(sel)

    traceprov_dump_safe_results(parsed.suff, dict(result=result))


if __name__ == "__main__":
    run()
