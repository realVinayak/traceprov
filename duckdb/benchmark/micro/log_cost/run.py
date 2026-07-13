from itertools import product
import os
from pathlib import Path

from traceprovpy.tools.benchmark_utils import LogCostBench, traceprov_dump_safe_results
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse, traceprov_handle_suffix
from traceprovpy.tools.file_utils import get_tmp_file, json_read_file, just_write, traceprov_assert_safe_run
from traceprovpy.tools.run_duckdb_generic import run_combined, run_single_query_dry, set_extra_traceprov_options

def simple_select_clause(num_rows: int, num_cols: int):
    columns = ','.join([f"column_{col}" for col in range(1, num_cols+1)])
    select_clause_stmt = f"select {columns} from {LogCostBench.get_table(num_rows)}"
    return select_clause_stmt

def capture_select_clause(num_rows: int, num_cols: int):
    columns = ','.join([f"column_{col}::int" for col in range(1, num_cols+1)])
    select_clause_stmt = f"select {columns}, traceprov_log_entry_{num_cols}(1, {columns}) from {LogCostBench.get_table(num_rows)}"
    return select_clause_stmt

def set_extra(parsed, table_count):
    extra_options = {
        "custom_graph_type": 1,
        "log_chain_table_count": table_count,
        "min_layer_number": 2,
    }
    flat = [f"--{key} {value}" for (key, value) in extra_options.items()]
    combined = " ".join(flat)
    set_extra_traceprov_options(parsed, combined)

def run():
    base_parser = make_duckdb_parse()
    base_parser.add_argument('--config', required=True)
    parsed = base_parser.parse_args()
    traceprov_handle_suffix(parsed)
    config = json_read_file(parsed.config)
    num_rows = config['num_rows']
    num_cols = config['num_cols']
    run_time_options = config["runTimeOptions"]
    total_iters = run_time_options["repeat"] + run_time_options["throwaway"]
    max_num_cols = max(num_cols)
    tmp_dir = Path(get_tmp_file())
    # don't do infer
    parsed.infer = False
    # don't do sample infer.
    parsed.sample_inference = None
    for row_count in num_rows:
        # dump a table with row_count rows and max_num_cols columns
        create_table_stmt = LogCostBench.create_table_clause(row_count, max_num_cols)
        create_table_path = just_write(tmp_dir / f"create_{row_count}.sql", create_table_stmt)
        exec_str, base_options = run_single_query_dry(
            create_table_path,
            parsed,
            iters=1
        )
        traceprov_assert_safe_run(f"{exec_str} {base_options.serialize()}")
    parsed.base_root = tmp_dir.as_posix()
    parsed.root = tmp_dir.as_posix()
    query_name = "query"
    q_out_dir = tmp_dir / query_name
    os.makedirs(q_out_dir, exist_ok=True)
    is_traceprov = parsed.sd_mode is None
    results = []
    for row_count, column_count in product(num_rows, num_cols):
        base_sql = simple_select_clause(row_count, column_count)
        capture_new_compact_sql = capture_select_clause(row_count, column_count)
        just_write(q_out_dir / "base.sql", base_sql)
        just_write(q_out_dir / "capture_new_compact.sql", capture_new_compact_sql)
        if is_traceprov:
            set_extra(parsed, column_count)
        query_result = run_combined(parsed, total_iters, query_name, [], 0)
        results.append(
            dict(row_count=row_count, column_count=column_count, result=query_result)
        )
        set_extra_traceprov_options(parsed, "")
    mode = "SmokedDuck" if not is_traceprov else "TraceProv"
    mode_result = dict(mode=mode, results=results)
    traceprov_dump_safe_results(parsed.suff, dict(result=mode_result))

if __name__ == '__main__':
    run()
