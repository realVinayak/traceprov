import os
from typing import NamedTuple, Set
import time
import json

WORKER_ID = "column_0"
LOCAL_GROUP_NUMBER = "column_1"
GLOBAL_GROUP_NUMBER = "column_2"

PREPARE_DUCKDB = """
#!/bin/bash

sudo -s <<EOF
cp /var/lib/postgresql/14/main/traceprov/worker_* $1
EOF
"""

BENCH_DUCKDB = """
SET enable_profiling = 'json'; 
SET profile_output = '/tmp/duckdb_profile.json';
"""


class ParquetRepr(NamedTuple):
    table_name: str
    alias: str

    def repr(self):
        return f"{self.table_name} AS {self.alias}"

    def __str__(self):
        return self.alias

    def __repr__(self):
        return self.alias


MAKE_PARQUET = lambda table: ParquetRepr(f"'{table}.parquet'", table)


def get_combine_join(main_worker, worker):
    final_layer = MAKE_PARQUET(f"worker_{main_worker}_layer_1")
    combine_layer = MAKE_PARQUET(f"worker_{main_worker}_layer_2")
    base_layer = MAKE_PARQUET(f"worker_{worker}_layer_0")
    sql = (
        f"select {base_layer}.column_1 "
        f"from {final_layer.repr()} join {combine_layer.repr()} on {combine_layer}.{GLOBAL_GROUP_NUMBER} = {final_layer}.column_0 "
        f"join {base_layer.repr()} on {base_layer}.column_0 = {combine_layer}.{LOCAL_GROUP_NUMBER} and {combine_layer}.{WORKER_ID} = {worker}"
    )
    return sql


def make_combines(main_worker_id: int, workers: Set[int]):
    sql_stmts = [
        f"({get_combine_join(main_worker_id, worker_id)})" for worker_id in workers
    ]
    return sql_stmts


def make_sql(main_worker_id: int | None, workers: Set[int]):
    sql_stmts = [] if main_worker_id is None else make_combines(main_worker_id, workers)
    main_worker_id = main_worker_id if main_worker_id is not None else list(workers)[0]
    base_layer = MAKE_PARQUET(f"worker_{main_worker_id}_layer_0")
    final_layer = MAKE_PARQUET(f"worker_{main_worker_id}_layer_1")
    sql = (
        f"select {base_layer}.column_1 "
        f"from {base_layer.repr()} join {final_layer.repr()} on {base_layer}.column_0 = {final_layer}.column_0"
    )
    sql_stmts = [*sql_stmts, sql]
    sql_stmts = [f"({sql})" for sql in sql_stmts]
    sql_stmts_unioned = "UNION ALL".join(sql_stmts)
    return f"select * from ({sql_stmts_unioned}) AS G;"


def perform_duckdb_inference(mode=0):
    use_temporary = mode == 0
    use_table = mode <= 1

    with open("/tmp/prepare_for_duckdb.sh", "w") as f:
        f.write(PREPARE_DUCKDB)

    os.system("chmod +x /tmp/prepare_for_duckdb.sh")

    db_inference_dir = "./playground/duckdb_inference/"
    os.system(f"rm -rf {db_inference_dir}")
    os.system("rm -f /tmp/duckdb_profile.json")
    os.makedirs(f"{db_inference_dir}")

    os.system(f"/tmp/prepare_for_duckdb.sh {os.getcwd()}/{db_inference_dir}")
    os.system(f"sudo chown -R realvinayak123:realvinayak123 {db_inference_dir}")

    for root, dirs, files in os.walk(db_inference_dir):
        file_list = list(files)

    main_worker_id = None
    worker_set = set()
    for file in file_list:

        file_prefix = file.split(".")[0]
        split = file_prefix.split("_")
        worker = int(split[1])
        layer = int(split[3])

        if layer == 2:
            assert main_worker_id is None
            main_worker_id = worker

        worker_set.add(worker)
        os.system(
            f"duckdb -c \"COPY (SELECT * FROM read_csv('{db_inference_dir}/{file}')) TO '{db_inference_dir}/{file_prefix}.parquet' (FORMAT PARQUET, COMPRESSION UNCOMPRESSED);\""
        )

    print(worker_set)
    print(main_worker_id)
    sql = make_sql(main_worker_id, worker_set)
    if use_table:
        sql = f"create or replace {'temp' if use_temporary else ''} table test_table as {sql}"
    print("###############################SQL###########################")
    print(sql)
    print("###############################SQL###########################")
    count = None
    if mode < 3:
        sql_file = f"{BENCH_DUCKDB}\n{sql}"
        with open("/tmp/duckdb_script.sql", "w") as f:
            f.write(sql_file)
        os.system(f"cd {db_inference_dir} && duckdb test.db -f /tmp/duckdb_script.sql")
        with open("/tmp/duckdb_profile.json") as f:
            profile = json.loads(f.read())
            time_taken = profile["latency"]
        if use_table and not use_temporary:
            os.system(
                f'cd {db_inference_dir} && duckdb test.db -csv -noheader -c "select count(*) from test_table" > /tmp/duckdb_count.txt'
            )
            with open("/tmp/duckdb_count.txt") as f:
                count = int(f.read().strip())
    else:
        with open("/tmp/duckdb_script.sql", "w", encoding="utf-8") as f:
            f.write(sql)
        assert (
            os.system(
                f"cd {db_inference_dir} && cat /tmp/duckdb_script.sql | perform_duckdb_inference"
            )
            == 0
        )
        with open("/tmp/duckdb_cpp_inference.txt") as f:
            time_taken = int(f.read().strip())

        with open("/tmp/duckdb_cpp_inference_size.txt") as f:
            count = int(f.read().strip())

    result = dict(time=time_taken, count=count)

    return result


if __name__ == "__main__":
    print(perform_duckdb_inference())
