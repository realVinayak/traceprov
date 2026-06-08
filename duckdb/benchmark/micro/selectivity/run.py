import argparse
import os
from pathlib import Path
from traceprovpy.tools.parser_defs import make_duckdb_selectivity_parser
from traceprovpy.utils import get_filter_group
from utils import make_replacer
from traceprovpy.tools.benchmark_utils import (
    traceprov_dump_safe_results,
)
from traceprovpy.tools.duckdb_parse_options import (
    traceprov_handle_suffix,
)
from traceprovpy.tools.run_duckdb_generic import (
    json_read_file,
    just_read,
    just_write,
    run_combined,
    set_extra_traceprov_options,
)

import random

# to make sampling reproducible.
random.seed(10)


def run():
    parsed = make_duckdb_selectivity_parser().parse_args()
    traceprov_handle_suffix(parsed)

    result = []
    tmp = Path("./tmp/")
    os.makedirs(tmp, exist_ok=True)

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
    run_time_options = config["runTimeOptions"]
    total_iters = run_time_options["repeat"] + run_time_options["throwaway"]
    query = "query"
    os.makedirs(tmp / query, exist_ok=True)
    parsed.root = tmp.as_posix()
    parsed.base_root = tmp.as_posix()
    replace_clause = ":selectivity" if not parsed.top_k_mode else ":top_k_limit"
    for query_dir in config["dirs"]:

        def run(raw_selectivity):
            selectivity = get_filter_group(
                parsed.num_groups, raw_selectivity, parsed.mode
            )
            replacer = make_replacer(
                str(query_dir["num_rows"]), selectivity, parsed.random, replace_clause
            )

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
            if parsed.sd_mode:
                set_extra_traceprov_options(parsed, "--disable_chunk_cache")
            query_result = run_combined(parsed, total_iters, query, [])
            result.append(
                dict(
                    dir=query_dir["num_rows"],
                    selectivity=selectivity,
                    result=query_result,
                    optimized=parsed.optimized,
                )
            )
            if parsed.sd_mode:
                set_extra_traceprov_options(parsed, "")

        for sel in query_dir["sel"]:
            run(sel)

    traceprov_dump_safe_results(parsed.suff, dict(result=result))


if __name__ == "__main__":
    run()
