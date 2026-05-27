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
from traceprovpy.tools.file_utils import json_read_file, just_read, just_write
from traceprovpy.tools.run_with_timeout import ConnectionParams

import re

needs_unnest = ["02", "17", "15", "11"]
needs_lateral = ["02", "17"]

# needs_unnest = []
# needs_lateral = []


def get_file(options: GpromOptions, parsed):
    flat = ["gprom", options.mode]
    if options.heuristics:
        flat.append("heuristics")
    combined = "_".join(flat)
    return f"{combined}.sql"


import sys


def handle_special_cases(parsed, query_path: Path, query_num: str):
    # Previously, I was against an approach like this, because it is hacky.
    # for this case, it is fine (just the extraction -- all the other places handle it the old way)
    if int(query_num) != 11 or parsed.sf != 10:
        return query_path
    sf_10_value = "0.0000100000"
    sf_1_value = "0.0001000000"
    query_contents = just_read(query_path)
    occur_count = query_contents.count(sf_1_value)
    assert occur_count == 1, f"Got {occur_count} occurences"
    query_contents = query_contents.replace(sf_1_value, sf_10_value)
    os.makedirs("./tmp/adjusted", exist_ok=True)
    return just_write("./tmp/adjusted/query_11_adjusted.sql", query_contents)


def handle_keys(parsed, query_path: Path, in_query_num: str):
    if not parsed.add_keys:
        return query_path
    raw_file = just_read(query_path)
    all_keys = json_read_file(Path(parsed.source) / "keys.json")
    assert all_keys
    for query_num, query_keys in all_keys.items():
        if int(query_num) == int(in_query_num):
            break
    else:
        raise Exception(f"Expected to find: {in_query_num}")
    regexes = [
        (r"FROM\s*\(\n*\s*PROVENANCE", "FROM"),
        (r"from\s*\(\n*\s*PROVENANCE", "from"),
    ]
    for regex in regexes:
        if re.search(regex[0], raw_file):
            break
    else:
        raise Exception(f"Expected to match either in {query_path}")
    split = raw_file.split(regex[1], maxsplit=1)
    assert len(split) == 2
    with_keys = ",".join([split[0], *query_keys])
    new_sql = [with_keys, " from ", split[1]]
    query_contents = "\n".join(new_sql)
    return just_write(f"/tmp/query_keys_{in_query_num}.sql", query_contents)


def main():
    parser = argparse.ArgumentParser(prog="gprom-tpch-query-gen")
    parser.add_argument("--source", required=True, type=str)
    parser.add_argument("--dest", type=str)
    parser.add_argument(
        "--is_all", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--original", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--add_keys", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--optimized", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument("--sf", type=int, required=True)
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
    # queries = ["22"]
    passed = defaultdict(dict)
    for query in queries:
        gprom_suffixes = ["extract"]
        if parsed.is_all:
            gprom_suffixes.append("all")
        if parsed.original:
            gprom_suffixes.append("original")
        if parsed.optimized:
            gprom_suffixes.append("optimized")
        path = "_".join(gprom_suffixes)
        absolute_input_path = Path(parsed.source) / f"{query}.gprom.{path}.sql"
        if not absolute_input_path.exists():
            print("Skipping: ", absolute_input_path)
            continue
        # assert absolute_input_path.exists(), f"Expected {absolute_input_path} to exist"
        absolute_input_path = handle_special_cases(parsed, absolute_input_path, query)
        absolute_input_path = handle_keys(parsed, absolute_input_path, query)
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

    out_path = (
        None
        if parsed.dest is None
        else Path(parsed.dest) / parsed.backend / str(parsed.sf)
    )
    for query, query_options in passed.items():
        for option, query_contents in query_options.items():
            file_name = get_file(option, parsed)
            if out_path:
                abs_file_path: Path = out_path / str(int(query)) / file_name
                os.makedirs(abs_file_path.parent, exist_ok=True)
                just_write(abs_file_path, query_contents["query"])
            print(get_file(option, parsed))

    if out_path:
        config_file: Path = out_path / "config_gprom.json"
        passed_remap = {
            query: {
                option.to_str(): dict(passed=option_data["passed"])
                for option, option_data in query_options.items()
            }
            for (query, query_options) in passed.items()
        }
        passed_remap["call_mode"] = dict(
            all=parsed.is_all,
            original=parsed.original,
            is_keys=parsed.add_keys,
            optimized=parsed.optimized,
        )
        just_write(config_file, json.dumps(passed_remap))

    # print(json.dumps(passed))


if __name__ == "__main__":
    main()
