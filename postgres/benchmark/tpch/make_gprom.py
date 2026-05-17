import argparse
from collections import defaultdict
from email.policy import default
import json
import os
from pathlib import Path

from traceprovpy.tools.connection_utils import (
    duckdb_connection_from_cmd,
    postgres_connection_from_cmd,
)
from traceprovpy.tools.extract_gprom_simple import (
    GPROM_OPTIONS_MAPPING,
    GpromOptions,
    gprom_from_file,
)
from traceprovpy.tools.file_utils import just_write
from traceprovpy.tools.run_with_timeout import ConnectionParams

needs_unnest = ["02", "17", "15", "22", "11"]
needs_lateral = ["02", "17"]

# needs_unnest = []
# needs_lateral = []


def get_file(options: GpromOptions, parsed):
    flat = ["gprom", options.mode]
    if options.heuristics:
        flat = [*flat, "heuristics"]
    if parsed.is_all:
        flat = [*flat, "all"]
    combined = "_".join(flat)
    return f"{combined}.sql"


import sys


def main():
    parser = argparse.ArgumentParser(prog="gprom-tpch-query-gen")
    parser.add_argument("--source", required=True, type=str)
    parser.add_argument("--dest", type=str)
    parser.add_argument(
        "--is_all", action=argparse.BooleanOptionalAction, default=False
    )
    GpromOptions.add_parse_options(parser)
    curr_args = " ".join(sys.argv)
    print("Handling: ", curr_args)
    if "--backend duckdb" in curr_args:
        duckdb_connection_from_cmd(parser)
    elif "--backend postgres" in curr_args:
        postgres_connection_from_cmd(parser)
    else:
        assert False, "Invalid backend!"
    parsed, _ = parser.parse_known_args()
    connection_params = ConnectionParams.make_from_parsed(
        parsed, backend=parsed.backend
    )
    queries = [str(q).rjust(2, "0") for q in range(1, 23)]
    # queries = ["04"]
    # queries = ["11"]
    passed = defaultdict(dict)
    for query in queries:
        if parsed.is_all:
            absolute_input_path = Path(parsed.source) / f"{query}.gprom.extract_all.sql"
        else:
            absolute_input_path = Path(parsed.source) / f"{query}.gprom.extract.sql"
        assert absolute_input_path.exists(), f"Expected {absolute_input_path} to exist"
        for gprom_mode in GPROM_OPTIONS_MAPPING:
            options = GPROM_OPTIONS_MAPPING[gprom_mode]
            if query in needs_lateral:
                options = options._replace(is_lateral=True)
            if query in needs_unnest:
                options = options._replace(is_unnest=True)
            options = options.from_parsed(parsed)
            try:
                gprom_sql = gprom_from_file(
                    options, connection_params, absolute_input_path
                )
                print(gprom_sql)
                val_pack = dict(query=gprom_sql, passed=True)
                passed[query][options] = val_pack
                con = connection_params.make_connection()
                try:
                    cursor = con.cursor()
                    cursor.execute(f"EXPLAIN {gprom_sql}")
                    cursor.close()
                    con.close()
                    print(f"Passed explain check!: ", query, options)
                except:
                    val_pack["passed"] = False
                    con.rollback()
                finally:
                    con.close()
            except Exception as e:
                print(e)
                print(f"Failed explain check!: ", query, options)

    for query, query_options in passed.items():
        for option, query_contents in query_options.items():
            file_name = get_file(option, parsed)
            if parsed.dest:
                dest = Path(parsed.dest)
                abs_file_path: Path = dest / str(int(query)) / file_name
                os.makedirs(abs_file_path.parent, exist_ok=True)
                just_write(abs_file_path, query_contents["query"])
            print(get_file(option, parsed))

    if parsed.dest:
        config_file: Path = Path(parsed.dest) / "config_gprom.json"
        passed_remap = {
            query: {
                option.to_str(): dict(passed=option_data["passed"])
                for option, option_data in query_options.items()
            }
            for (query, query_options) in passed.items()
        }
        just_write(config_file, json.dumps(passed_remap))

    # print(json.dumps(passed))


if __name__ == "__main__":
    main()
