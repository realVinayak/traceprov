import os
from pathlib import Path
import random

from traceprovpy.tools.benchmark_utils import traceprov_dump_safe_results
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse
from traceprovpy.tools.file_utils import json_read_file, just_read, just_write
from traceprovpy.tools.run_duckdb_generic import create_base_offset, run_combined
random.seed(10)

def run_count_query(num_rows: int, query: str, parsed, tmp: Path, total_iters):
    query_dir = Path(f"queries/{query}/")
    base_sql = just_read(query_dir / "base.sql")
    create_base_offset(query_dir)
    base_offset_sql = just_read(query_dir / "base_offset.sql")
    capture_sql = just_read(query_dir / "capture.sql")
    capture_new_sql = just_read( query_dir / "capture_new.sql")
    capture_new_compact_sql = just_read( query_dir / "capture_new_compact.sql")
    validate_sql = just_read(query_dir / "validate.sql")
    validate_new_sql = just_read(query_dir / "validate_new.sql")
    validate_offset_sql = just_read(query_dir / "validate_offset.sql")
    validate_new_offset_sql = just_read(query_dir / "validate_new_offset.sql")
    validate_sd_sql = just_read(query_dir / "validate_sd.sql")

    def replacer(in_sql: str):
        return in_sql.replace("ROW_COUNT", str(num_rows))

    out_dir = tmp / query
    os.makedirs(out_dir, exist_ok=True)
    just_write(out_dir / "base.sql", replacer(base_sql))
    just_write(out_dir / "base_offset.sql", replacer(base_offset_sql))
    just_write(out_dir / "capture.sql", replacer(capture_sql))
    just_write(out_dir / "capture_new.sql", replacer(capture_new_sql))
    just_write(out_dir / "capture_new_compact.sql", replacer(capture_new_compact_sql))
    just_write(out_dir / "validate.sql", replacer(validate_sql))
    just_write(out_dir / "validate_sd.sql", replacer(validate_sd_sql))
    just_write(
        out_dir / "validate_offset.sql", replacer(validate_offset_sql)
    )
    just_write(
        out_dir / "validate_new_offset.sql",
        replacer(validate_new_offset_sql),
    )
    just_write(out_dir / "validate_new.sql", replacer(validate_new_sql))
    query_result = run_combined(parsed, total_iters, query)
    return query_result



def run():
    base_parser = make_duckdb_parse()
    base_parser.add_argument("--config", required=True)
    parsed = base_parser.parse_args()

    result = []
    tmp = Path("./tmp/")
    os.makedirs(tmp, exist_ok=True)

    config = json_read_file(parsed.config)
    parsed.base_root = tmp.as_posix()
    parsed.root = tmp.as_posix()
    for element in config['dirs']:
        num_rows = str(int(element['num_rows']))
        queries = element["queries"]
        if (queries == "_all_"):
            queries = ["q01", "q02", "q03", "q04", "q06", "q07"]
        for query in queries:
            query_result = run_count_query(num_rows, query, parsed, tmp, config['repeat'])
            result.append(
                dict(
                    dir=num_rows,
                    query=query,
                    result=query_result,
                    optimized=parsed.optimized,
                )
            )
    
    traceprov_dump_safe_results(parsed.suff, dict(result=result))

if __name__ == "__main__":
    run()