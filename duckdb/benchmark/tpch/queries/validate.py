import argparse
from datetime import datetime
import json
import os
from pathlib import Path

from gen_query import get_in_extra
from traceprovpy.tools.benchmark_utils import traceprov_assert_safe_run
from traceprovpy.tools.duckdb_inference import DuckDBDriverOptions

query_list = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 17, 18, 19, 20, 21, 22]
needs_disable = {11, 17, 18, 2, 20, 22}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--db", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--qnum")

    parsed = parser.parse_args()
    with open("spec_2.json") as f:
        spec = json.loads(f.read())["result"]["params_default"]
    reduced = {
        str(qnum): dict(
            capture=get_in_extra(qdata, "traceprov_capture_query"),
            derive=json.loads(get_in_extra(qdata, "traceprov_generic_derivation_spec"))[
                "elements"
            ],
            min_local_used=json.loads(
                get_in_extra(qdata, "traceprov_generic_derivation_spec")
            )["min_local_used"],
        )
        for (qnum, qdata) in spec.items()
    }

    root = Path("./root")
    for query_num in query_list:
        query_num_str = str(query_num)
        if parsed.qnum is not None and parsed.qnum != query_num_str:
            continue
        path = root / Path(query_num_str)
        base_sql = path / "base.sql"
        with open(base_sql) as f:
            base_query = f.read()
            base_filled_in = path / "base_filled.csv"
            base_query = base_query.replace(";", "")
            base_query = f"copy (select * from ({base_query}) f) to '{base_filled_in.as_posix()}' (header false)"
        base_csv = path / "base_dump.sql"
        with open(base_csv, "w") as f:
            f.write(base_query)
        validate_sql = path / "validate.sql"
        validate_filled_sql = path / "validate_filled.sql"
        assert validate_sql.exists(), f"not found: {validate_sql.as_posix()}"
        with open(validate_sql) as f:
            validate_sql_file = f.read()
        query_data = reduced[query_num_str]
        with open(validate_filled_sql, "w") as f:

            base_options = DuckDBDriverOptions(
                db=parsed.db,
                repeat=1,
                threads=1,
                i=base_csv.as_posix(),
                min_layer_number=query_data["min_local_used"],
            )
            derived_contents = reduced[query_num_str]["derive"]
            for element in derived_contents:
                el_id = element["idx"]
                ticker = f"LAYER_{el_id}"
                with open("/tmp/drop_table.sql", "w") as new_f:
                    new_f.write(f"DROP table if exists {ticker}")
                drop_option = base_options._replace(i="/tmp/drop_table.sql")
                infer_out = path / f"infer_{el_id}_out.csv"
                traceprov_assert_safe_run(f"{parsed.exe} {drop_option.serialize()}")
                with open("/tmp/create.sql", "w") as new_f:
                    new_f.write(
                        f"create table {ticker} as (select * from '{infer_out.absolute().as_posix()}')"
                    )
                create_option = base_options._replace(i="/tmp/create.sql")
                traceprov_assert_safe_run(f"{parsed.exe} {create_option.serialize()}")
                validate_sql_file = validate_sql_file.replace(f"'{ticker}'", ticker)

            filled_in_csv = path / "data_filled.csv"
            validate_sql_file = f"copy (select * from ({validate_sql_file}) f) to '{filled_in_csv.as_posix()}' (header false)"
            print(validate_sql_file)
            validate_sql_file = validate_sql_file.replace(";", "")
            f.write(validate_sql_file)

        base_options = DuckDBDriverOptions(
            db=parsed.db,
            repeat=1,
            threads=1,
            i=base_csv.as_posix(),
            min_layer_number=query_data["min_local_used"],
        )
        validate_options = base_options._replace(i=validate_filled_sql.as_posix())
        traceprov_assert_safe_run(f"{parsed.exe} {validate_options.serialize()}")
        traceprov_assert_safe_run(f"{parsed.exe} {base_options.serialize()}")
        assert base_filled_in.as_posix() != filled_in_csv.as_posix()
        traceprov_assert_safe_run(
            f"diff {base_filled_in.as_posix()} {filled_in_csv.as_posix()}"
        )


if __name__ == "__main__":
    main()
