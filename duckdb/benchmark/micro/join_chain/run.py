from functools import reduce
import os
from pathlib import Path

from traceprovpy.tools.benchmark_utils import traceprov_dump_safe_results
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
import duckdb
from traceprovpy.tools.run_duckdb_generic import (
    run_combined,
    set_extra_traceprov_options,
)

MAIN_COLUMN = "idx"


def series_table_name(table_prefix, idx):
    return f"{table_prefix}_series_table_{idx}"


def single_row_table_name(table_prefix, idx):
    return f"{table_prefix}_single_row_table_{idx}"


def multiple_row_table_name(table_prefix, idx):
    return f"{table_prefix}_multiple_row_table_{idx}"


def create_table_generic(con, table_name, select_clause):
    con.execute(f"create table {table_name} as ({select_clause})")
    # con.execute("commit;")


def create_single_row_table(table_prefix, con, idx):
    table_name = single_row_table_name(table_prefix, idx)
    create_table_generic(con, table_name, f"select 1 as {MAIN_COLUMN}")


def create_multiple_row_table(table_prefix, con, idx, row_count):
    table_name = multiple_row_table_name(table_prefix, idx)
    create_table_generic(
        con,
        table_name,
        f"select 1 as {MAIN_COLUMN} from generate_series(1, {row_count})",
    )


def create_series_table(table_prefix, con, idx, fan_out):
    table_name = series_table_name(table_prefix, idx)
    create_table_generic(
        con,
        table_name,
        f"select generate_series as {MAIN_COLUMN} from generate_series(1, {fan_out})",
    )


def make_table(con, table_prefix: str, table_count: int, fan_out: int):
    total_row_count = fan_out
    for table_idx in range(table_count):
        # create single-row tables.
        create_single_row_table(table_prefix, con, table_idx)
        # create series tables
        create_series_table(table_prefix, con, table_idx, fan_out)
    # create terminal multiple row tables
    create_multiple_row_table(table_prefix, con, 0, total_row_count)


def _reducer(prev, curr):
    return f"{prev} join {curr} using ({MAIN_COLUMN})"


def make_join_query(table_names):
    from_clause = reduce(_reducer, table_names)
    query = f"select * from {from_clause}"
    return query


def make_traceprov_query(table_names, parsed):
    from_clause = reduce(_reducer, table_names)
    columns = [
        (
            f"{table_name}.rowid::int"
            if getattr(parsed, "traceprov_use_compact", False)
            else f"{table_name}.rowid"
        )
        for table_name in table_names
    ]
    column_combined = ",".join(columns)
    query = f"select traceprov_log_entry_{len(columns)} (1, {column_combined}) from {from_clause}"
    return query


def make_series_query(parsed, table_prefix, table_count: int):
    table_names = [series_table_name(table_prefix, idx) for idx in range(table_count)]
    return make_join_query(table_names), make_traceprov_query(table_names, parsed)


def make_single_row_query(parsed, table_prefix, table_count, flipped=False):
    table_names = [
        *(single_row_table_name(table_prefix, idx) for idx in range(table_count - 1)),
        multiple_row_table_name(table_prefix, 0),
    ]
    if flipped:
        table_names = table_names[::-1]
    return make_join_query(table_names), make_traceprov_query(table_names, parsed)


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
    base_parser.add_argument("--config", required=True)
    parsed = base_parser.parse_args()
    traceprov_handle_suffix(parsed)
    assert hasattr(parsed, "sample_inference")
    assert hasattr(parsed, "infer")
    parsed.sample_inference = None
    parsed.infer = False

    tmp = Path("./tmp/")
    os.makedirs(tmp, exist_ok=True)
    config = json_read_file(parsed.config)
    query_dirs = config["query_dir"]

    total_iters = get_total_iters(config)

    db_path: Path = Path(parsed.db)
    traceprov_assert_safe_run(f"rm -rf {db_path.as_posix()}")
    parsed.root = tmp.as_posix()
    parsed.base_root = tmp.as_posix()

    results = []

    for query_dir_idx, query_dir in enumerate(query_dirs):
        table_count = query_dir["table_count"]
        fan_out = query_dir["fan_out"]
        table_prefix = f"dir_{query_dir_idx}"
        connection = duckdb.connect(db_path)
        cursor = connection.cursor()
        make_table(cursor, table_prefix, table_count, fan_out)
        series_query, traceprov_series_query = make_series_query(
            parsed, table_prefix, table_count
        )
        single_row_query, traceprov_single_row_query = make_single_row_query(
            parsed, table_prefix, table_count, False
        )
        flipped_single_row_query, traceprov_flipped_single_row_query = (
            make_single_row_query(parsed, table_prefix, table_count, True)
        )
        queries = ["series", "single_row", "flipped_single_row"]
        series_dir = tmp / queries[0]
        single_row_query_dir = tmp / queries[1]
        flipped_single_row_dir = tmp / queries[2]
        os.makedirs(series_dir, exist_ok=True)
        os.makedirs(single_row_query_dir, exist_ok=True)
        os.makedirs(flipped_single_row_dir, exist_ok=True)
        just_write(series_dir / "base.sql", series_query)
        just_write(single_row_query_dir / "base.sql", single_row_query)
        just_write(flipped_single_row_dir / "base.sql", flipped_single_row_query)
        add_compact = lambda name: (
            f"{name}_compact" if parsed.traceprov_use_compact else name
        )
        tp_name = add_compact("capture_new")
        tp_dest = f"{tp_name}.sql"
        just_write(series_dir / tp_dest, traceprov_series_query)
        just_write(single_row_query_dir / tp_dest, traceprov_single_row_query)
        just_write(flipped_single_row_dir / tp_dest, traceprov_flipped_single_row_query)
        dir_result = dict()
        cursor.close()
        connection.close()
        set_extra(parsed, table_count)
        for query in queries:
            query_result = run_combined(parsed, total_iters, query, [])
            dir_result[query] = query_result
        results.append(dict(dir=query_dir, result=dir_result))

    traceprov_dump_safe_results(parsed.suff, dict(result=results))


if __name__ == "__main__":
    run()
