# LOL, just a wrapper around TraceProv, because why not.

from pathlib import Path

from run import run
from traceprovpy.tools.file_utils import json_read_file

import duckdb


def make_create_table(table_name, original_table, order_spec):
    print(order_spec)
    layer_number = order_spec[0]
    match_column = order_spec[1]
    assert len(order_spec) == 2
    order_columns = ",".join([f"F.{col}" for col in order_spec[1:]])
    sql = f"""create or replace table {table_name} as (
        select base.* from {original_table} base left join (
            select distinct {match_column} as lineage_column from traceprov_lineage_{layer_number}
            ) F
        on base.rowid is not distinct from F.lineage_column order by F.lineage_column
    );
    """
    return sql


def hook(query_result_pack):
    parsed, query, _ = query_result_pack
    order_spec = json_read_file(Path(parsed.order_spec) / f"{query}.json")
    assert order_spec is not None

    db = Path(parsed.db)
    connection = duckdb.connect(db)
    cursor = connection.cursor()
    for order_spec_entry in order_spec:
        create_table_sql = make_create_table(
            order_spec_entry["updated"],
            order_spec_entry["original"],
            order_spec_entry["order_by"],
        )
        cursor.execute(create_table_sql)
    cursor.close()
    connection.close()


if __name__ == "__main__":
    run(force_materialize=True, execution_hook=hook)
