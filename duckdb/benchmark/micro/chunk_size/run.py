from functools import reduce
import os
from pathlib import Path
import duckdb

from traceprovpy.tools.benchmark_utils import (
    evaluate_if_str,
    traceprov_dump_safe_results,
)
from traceprovpy.tools.duckdb_parse_options import (
    make_duckdb_parse,
    traceprov_handle_suffix,
)
from traceprovpy.tools.file_utils import (
    get_total_iters,
    json_read_file,
    just_write,
    traceprov_assert_safe_run,
)
from traceprovpy.tools.run_duckdb_generic import (
    run_combined,
    run_single_query_dry,
    set_extra_traceprov_options,
)


def _make_table(previous, current):
    index, chunk_size, table_size, sel = current
    if len(previous) == 0:
        last_size = table_size
    else:
        last_size = previous[-1][2]
    return [*previous, (index + 1, chunk_size, table_size, last_size, sel)]


def _make_table_from_arg(options: tuple):
    print(options)
    arg_str = ",".join(map(str, list(options)))
    return f"traceprov_chunked_read({arg_str})"


def _make_table_name(table_prefix: str, table_index: int):
    return f"{table_prefix}_table_{table_index}"


def _make_create_table(clause: str, table_prefix: str, table_index: int):
    table_name = _make_table_name(table_prefix, table_index)
    return f"create table {table_name} as (from {clause})"


def make_table(
    parsed,
    tmp_path: Path,
    table_prefix: str,
    chunk_sizes: list[int],
    table_counts: list[int],
    selectivity: list[int],
):

    iterable = list(
        zip(range(len(chunk_sizes)), chunk_sizes, table_counts, selectivity)
    )
    table_args = list(map(_make_table_from_arg, reduce(_make_table, iterable, [])))
    for _idx, table_arg in enumerate(table_args):
        create_table_expr = _make_create_table(table_arg, table_prefix, _idx)
        create_table_path = just_write(
            tmp_path / "create_table_query.sql", create_table_expr
        )
        exec_str, options = run_single_query_dry(create_table_path, parsed, iters=1)
        options = options._replace(load_micro_benchmarks=True)
        traceprov_assert_safe_run(f"{exec_str} {options.serialize()}")


def _reduce_join(previous, current_table):
    previous_clause, previous_table = previous
    if previous_table is None:
        return current_table, current_table
    augmented_join = f"{previous_clause} join {current_table} on {previous_table}.column_1 = {current_table}.column_0"
    return augmented_join, current_table


def make_query(parsed, table_prefix, table_count):
    table_names = []
    for count_idx in range(table_count):
        table_names.append(_make_table_name(table_prefix, count_idx))
    table_names = table_names[::-1]
    from_clause, _ = reduce(_reduce_join, table_names, (None, None))
    normal_query = f"select count(*) from {from_clause}"
    if parsed.traceprov_use_compact:
        row_id_joined = ",".join([f"{table}.rowid::int" for table in table_names])
        traceprov_agg = (
            f"traceprov_agg_key_parallel_offset_{table_count}(1, {row_id_joined})"
        )
    else:
        row_id_joined = ",".join([f"{table}.rowid::bigint" for table in table_names])
        traceprov_agg = (
            f"traceprov_agg_key_parallel_offset_{table_count}(1, {row_id_joined})"
        )
    traceprov_query = (
        f"select count(*), traceprov_log_entry_1(2, {traceprov_agg}) from {from_clause}"
    )
    return normal_query, traceprov_query


def set_extra(parsed, table_count, extra_options: list):
    extra_options = {
        "custom_graph_type": 2,
        "log_chain_table_count": table_count,
        "min_layer_number": 3,
    }
    flat = [f"--{key} {value}" for (key, value) in extra_options.items()]
    combined = " ".join(flat) + " --sd_join_mode"
    set_extra_traceprov_options(parsed, combined)


def run():
    base_parser = make_duckdb_parse()
    base_parser.add_argument("--config", required=True)
    parsed = base_parser.parse_args()
    traceprov_handle_suffix(parsed)
    parsed.optimized = True

    tmp = Path("./tmp/")
    os.makedirs(tmp, exist_ok=True)
    config = json_read_file(parsed.config)
    query_dirs = config["query_dir"]

    total_iters = get_total_iters(config)
    db_path = Path(parsed.db)
    traceprov_assert_safe_run(f"rm -rf {db_path.as_posix()}")

    parsed.root = tmp.as_posix()
    parsed.base_root = tmp.as_posix()

    results = []

    for query_dir_idx, query_base_dir in enumerate(query_dirs):
        table_counts = list(map(evaluate_if_str, query_base_dir["table_counts"]))
        selectivity = list(map(evaluate_if_str, query_base_dir["selectivity"]))
        chunk_size = list(map(evaluate_if_str, query_base_dir["chunk_size"]))
        table_prefix = f"dir_{query_dir_idx}"
        make_table(parsed, tmp, table_prefix, chunk_size, table_counts, selectivity)
        normal_query, traceprov_query = make_query(
            parsed, table_prefix, len(table_counts)
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
        set_extra(parsed, len(table_counts))
        dir_result = dict()
        query_result = run_combined(parsed, total_iters, "query", [])
        dir_result["dir"] = query_base_dir
        dir_result["result"] = query_result
        results.append(dir_result)

    # print(results)
    traceprov_dump_safe_results(parsed.suff, dict(result=results))


if __name__ == "__main__":
    run()
