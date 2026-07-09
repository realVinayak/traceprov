from itertools import product
import os
from pathlib import Path

from traceprovpy.tools.benchmark_utils import (
    evaluate_list_if_str,
    traceprov_dump_safe_results,
)
from traceprovpy.tools.duckdb_parse_options import (
    make_duckdb_parse,
    traceprov_handle_suffix,
)
from traceprovpy.tools.file_utils import (
    get_tmp_file,
    get_total_iters,
    json_read_file,
    just_write,
    traceprov_assert_safe_run,
)
from traceprovpy.tools.run_duckdb_generic import run_combined, set_extra_base_options
from utils import set_extra

def _make_query(columns, table_clause):
    column_joined = ",".join(columns)
    return f"select {column_joined} {table_clause}"


def make_query(parsed, chunk_size, table_count, num_groups):
    table_clause = f"from traceprov_chunked_read(1, {chunk_size}, {table_count}, {num_groups}) group by column_1"
    columns = ["sum(column_0)", "column_1"]
    row_id = f"column_0"
    if parsed.traceprov_use_compact:
        row_id = f"{row_id}::int"
    traceprov_agg = f"traceprov_agg_key_parallel_offset_1(1, {row_id})"
    traceprov_log = f"traceprov_log_entry_1(2, {traceprov_agg})"
    tp_columns = ["sum(column_0)", traceprov_log]

    normal_query = _make_query(columns, table_clause)
    traceprov_query = _make_query(tp_columns, table_clause)
    return normal_query, traceprov_query


def run():
    base_parser = make_duckdb_parse()
    base_parser.add_argument("--config", required=True)
    parsed = base_parser.parse_args()
    traceprov_handle_suffix(parsed)
    parsed.optimized = True

    tmp = Path(get_tmp_file())
    os.makedirs(tmp, exist_ok=True)
    config = json_read_file(parsed.config)
    query_dirs = config["query_dir"]

    total_iters = get_total_iters(config)
    db_path = Path(parsed.db)
    traceprov_assert_safe_run(f"rm -rf {db_path.as_posix()}")

    parsed.root = tmp.as_posix()
    parsed.base_root = tmp.as_posix()

    results = []

    table_counts = evaluate_list_if_str(query_dirs["table_counts"])
    num_groups = evaluate_list_if_str(query_dirs["num_groups"])
    chunk_sizes = evaluate_list_if_str(query_dirs["chunk_size"])
    set_extra_base_options(parsed, "--load_micro_benchmarks")

    arg_pairs = product(table_counts, num_groups, chunk_sizes)
    for _, arg_pair in enumerate(arg_pairs):
        table_count, num_group, chunk_size = arg_pair
        normal_query, traceprov_query = make_query(
            parsed, chunk_size, table_count, num_group
        )
        query_dir = tmp / "query"
        os.makedirs(query_dir, exist_ok=True)
        just_write(query_dir / "base.sql", normal_query)
        add_compact = lambda name: (
            f"{name}_compact" if parsed.traceprov_use_compact else name
        )
        tp_name = add_compact("capture_new")
        tp_dest = f"{tp_name}.sql"
        just_write(query_dir / tp_dest, traceprov_query)
        set_extra(parsed, 1, ["load_micro_benchmarks"])
        dir_result = dict()
        query_result = run_combined(parsed, total_iters, "query", [])
        dir_result["dir"] = dict(
            table_count=table_count, num_group=num_group, chunk_size=chunk_size
        )
        dir_result["result"] = query_result
        results.append(dir_result)
    traceprov_dump_safe_results(parsed.suff, dict(result=results))


if __name__ == "__main__":
    run()
