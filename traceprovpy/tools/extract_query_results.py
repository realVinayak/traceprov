# script to extract the per-query results

import argparse
import json
import os
from pathlib import Path
import re
from typing import Any

from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse
from traceprovpy.tools.file_utils import json_read_file, just_write
from traceprovpy.tools.normalized_row import Extendable, Normalizable, NormalizedSampleInferRow, tap_profile_result, tap_simple_result
from traceprovpy.tools.run_duckdb_generic import add_query_options

VERSION_REG = r"^g_(\d+)_"


class TpchRow(Normalizable):
    category: str
    parallel: int
    query_num: str
    phase_1: Extendable
    phase_2: Extendable
    phase_1_profile: Extendable
    phase_2_profile: Extendable
    layer_number: int
    index_build_time: float
    sql_time: float
    partition_time: float
    version: int
    fail_reason: str

    def keys(self):
        return {
            "category", 
            "parallel",
            "query_num",
            "phase_1_profile",
            "phase_2_profile",
            "layer_number",
            "index_build_time",
            "sql_time",
            "partition_time",
            "version"
        }

# if it is sample then there's just offset to keep track off.
class TpchSampleRow(TpchRow):
    offset: int

    def keys(self):
        return super().keys() | {"offset"}

def parse_reg(in_str: str):
    match = re.match(VERSION_REG, in_str)
    assert match is not None
    groups = match.groups()
    assert len(groups) == 1
    group_version = int(groups[0].strip())
    return group_version

def get_first_value(res: dict):
    assert len(res) == 1
    return list(res.values())[0]

def get_error(result: dict):
    if 'timeout' in result:
        return "Timeout"
    if 'errorcode' in result:
        return result['error_content']
    return None

def flatten(rows: list[list[Any]]):
    return list([cell for row in rows for cell in row])

def handle_duckdb(db_system: str, mode: str, result_path: Path, rows_list: list):
    print("duckdb handling ", db_system, result_path)
    result = json_read_file(result_path)
    main_res: dict = result['results']
    terminal_name = result_path.name
    version = parse_reg(terminal_name)
    call_options = result['call_options']
    bench_parser = make_duckdb_parse()
    add_query_options(bench_parser)
    bench_parsed, _ = bench_parser.parse_known_args(call_options)
    thread_count = bench_parsed.threads

    def _handle_gprom_result(pack):
        query, gprom_mode, gprom_result = pack
        fail_reason = get_error(gprom_result)
        def_raw_args = dict(
            category=gprom_mode,
            query_num=query,
            parallel=thread_count,
            layer_number=0,
            index_build_time=0.0,
            sql_time=0.0,
            partition_time=0.0,
            version=version
        )
        kwarg_list = []
        if fail_reason is None:
            for offset_id, offset_result in enumerate(gprom_result):
                infer_result = list(map(tap_profile_result, offset_result['infer']['profile']))
                time_result = list(map(tap_simple_result,  offset_result['infer']['time']))
                kwargs = dict(
                    phase_1=Extendable(time_result),
                    phase_1_profile=Extendable(infer_result),
                    phase_2=None,
                    phase_2_profile=None
                )
                if mode == 'all':
                    kwargs = ({**kwargs, 'offset': offset_id})
                kwarg_list.append(kwargs)
        else:
            kwarg_list.append(
                dict(
                    phase_1=None,
                    phase_1_profile=None,
                    phase_2=None,
                    phase_2_profile=None
                )
            )
        _cls = TpchRow if mode == 'all' else TpchSampleRow
        rows = [
            _cls(**{**def_raw_args, **row_arg})
            for row_arg in kwarg_list
        ]
        return rows


    if db_system == 'gprom':
        rows = flatten([
            _handle_gprom_result(query, gprom_mode, gprom_result)
            for query, query_result in main_res.items()
            for gprom_mode, gprom_result in query_result.items()
        ])
    else:
        raise Exception("Not implemented anything else!")
    return rows

def handle_postgres(db_system: str, mode: str, result_path: Path, rows_list: list):
    print("postgres handling ", db_system, result_path)
    result = json_read_file(result_path)
    main_res: dict = get_first_value(result['result'])
    terminal_name = result_path.name
    version = parse_reg(terminal_name)
    for query, query_result in main_res.items():
        ...


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    parsed = parser.parse_args()
    postgres_rows = dict(offset=[], all=[])
    duckdb_rows = dict(offset=[], all=[])
    new_duckdb_rows = dict(offset=[], all=[])
    all_rows = dict(
        postgres=postgres_rows,
        duckdb=duckdb_rows,
        newduckdb=new_duckdb_rows
    )
    for dir_path, dirnames, filenames in os.walk(parsed.root):
        # print(dir_path, dirnames, filenames)
        if len(dirnames) == 0:
            print("LEAF: ", dir_path, dirnames, filenames)
            assert parsed.root in dir_path
            adjusted = dir_path.replace(parsed.root, '')
            adjusted_split = adjusted.split("/")[-4:]
            print(adjusted_split)
            db_name = adjusted_split[0]
            db_system = adjusted_split[-1]
            mode = adjusted_split[-2]
            print(db_name, db_system)
            print(list(sorted(filenames)))
            current_rows = all_rows[db_name][mode]
            handler = handle_duckdb if (db_name == 'duckdb' or db_name == 'newduckdb') else handle_postgres
            print(handler)
            for file in filenames:
                if 'main_result.json' in file or 'result.json' in file:
                    handler(db_system, mode, Path(dir_path) / file )
        else:
            ...

if __name__ == '__main__':
    main()