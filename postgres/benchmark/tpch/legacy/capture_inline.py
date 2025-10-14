from collections import defaultdict
import json
import time
import uuid
import os

DEFAULT_ID_TABLE_NAME = "captured_id"


def make_select(table, keys):
    return ",".join([f"{table}.{key}" for key in keys])


def create_table(ref_sql, table, args, is_temp=True):

    (layer, ref, subq, keys) = args

    # Need to, first, create the function.

    if len(keys) == 1:
        # Looks like postgres doesn't like when there is just one out.
        # Don't know why it should make a difference, but whatever.
        keys = [*keys, "sample_column"]

    suffix = str(uuid.uuid4()).split("-")[0]
    outs = ",".join([f"OUT {key} integer" for key in keys])

    func_sql = ref_sql.replace("%A%", suffix)
    func_sql = func_sql.replace("%OUT%", outs)

    function_name = f"traceprov_infer_{suffix}"

    sql = [
        f"-- TABLE : {table} -- ",
        f"DROP TABLE IF EXISTS {table};",
        func_sql,
        f"CREATE {'TEMP' if is_temp else ''} TABLE {table} AS SELECT * FROM {function_name}({layer}, {ref}, {subq});",
    ]

    return sql


def create_tables(ref_sql, tables, is_temp=True):
    combined_sql = []
    for table, args in tables.items():
        combined_sql = [*combined_sql, *create_table(ref_sql, table, args, is_temp)]
    return combined_sql


def run_validate(
    config_file_name,
    db_name,
    infer_template_sql_file,
    port,
    validate_sql_file=None,
    raw_sql_file=None,
):

    with open(config_file_name) as cf:
        config = json.loads(cf.read())

    layer_number = config.get("layer_number", 1)
    subq_table = config.get("subq_table_name")
    reference = config.get("reference", 0)
    subq_layer = None if subq_table is None else config.get("subq_layer")

    ID_TABLE_NAME = config.get("id_table_name", DEFAULT_ID_TABLE_NAME)

    tables = {ID_TABLE_NAME: (layer_number, reference, 0, config["pk_order"])}

    if "subq_pk_order" in config:
        assert subq_table and subq_layer
        tables[subq_table] = (0, 0, subq_layer, config["subq_pk_order"])

    with open(infer_template_sql_file) as itsf:
        ref_sql = itsf.read()

    create_table_sql = create_tables(ref_sql, tables, validate_sql_file is None)

    with open("/tmp/create_table_temp.sql", "w") as f:
        f.write("\n".join(create_table_sql))

    start = time.perf_counter()
    assert (
        os.system(
            f"PGPASSWORD=postgres psql -p {port} -U postgres {db_name} -f /tmp/create_table_temp.sql > /tmp/temp.out"
        )
        == 0
    )
    end = time.perf_counter()
    naive_duration = end - start

    if validate_sql_file is None:
        print("skipping validation", naive_duration)
        return naive_duration, 0

    with open(validate_sql_file) as vsf:
        sql_to_inject = vsf.read()

    for participating in config["inserts"]:
        ref = participating["ref"]
        keys = participating["keys"]
        is_subq = participating.get("is_sub", False)

        inner_table = subq_table if is_subq else ID_TABLE_NAME

        key_select_sql = f"SELECT {make_select(inner_table, keys)} FROM {inner_table}"
        sql_to_inject = sql_to_inject.replace(f"%{ref}%", key_select_sql)

    final_sql = [sql_to_inject]
    with open("/tmp/validate.sql", "w") as f:
        f.write("\n".join(final_sql))

    get_run_cmd = (
        lambda in_file, out_file: f"PGPASSWORD=postgres psql -p {port} -U postgres {db_name} -A --field-separator='|' -P \"footer=off\" -f {in_file} > {out_file}"
    )

    injected_run_cmd = get_run_cmd("/tmp/validate.sql", "/tmp/injected.out")
    assert os.system(injected_run_cmd) == 0

    raw_run_cmd = get_run_cmd(raw_sql_file, "/tmp/raw.out")

    assert os.system(raw_run_cmd) == 0
    print("Validating!")
    diff_return = os.system("diff -u /tmp/injected.out /tmp/raw.out")
    return None, diff_return


import sys

if __name__ == "__main__":
    run_validate(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5])
    ...
