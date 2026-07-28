# script to extract the per-query results

import argparse
import json
import os
from pathlib import Path
import re
from typing import Any, NamedTuple

#from duckdb.benchmark.tpch.gprom_results import reduce
from functools import reduce
from traceprovpy.tools.duckdb_inference import DuckDBDriverOptions
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse, traceprov_handle_suffix
from traceprovpy.tools.file_utils import json_read_file, just_write
from traceprovpy.tools.normalized_row import Extendable, Normalizable, NormalizedSampleInferRow, tap_profile_result, tap_simple_result, make_dummy_profile_result, make_dummy_simple_result, extract_infer, NORM_ITER_COL
from traceprovpy.tools.run_duckdb_generic import add_query_options
from traceprovpy.tools.run_duckdb_generic import (
    TRACEPROV_CAPTURE_ENTRY,
    TRACEPROV_CAPTURE_ENTRY_SD,
)
VERSION_REG = r"^g_(\d+)_"


class TpchRow(Normalizable):
    category: str
    parallel: int
    query_num: str
    phase_1: Extendable
    phase_2: Extendable
    phase_1_profile: Extendable
    phase_2_profile: Extendable
    log_sizes: Extendable
    layer_number: int
    version: int
    fail_reason: str
    extra: dict | Extendable

    def keys(self):
        return {
            "category", 
            "parallel",
            "query_num",
            "phase_1_profile",
            "phase_2_profile",
            "layer_number",
            "version",
            "phase_1",
            "phase_2",
            "fail_reason",
            "log_sizes",
            "extra"
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
    assert isinstance(result, list), f"Got result of type {type(result)}"
    for _result in result:
        if _result['type'] == 'fail':
            code = _result['code']
            return f"code_{code}"
    return None

def flatten(rows: list[list[Any]]):
    return list([cell for row in rows for cell in row])

def extract_str(in_regex):
    def _extract(in_str: int):
        matches = re.findall(in_regex, in_str)
        if len(matches) == 0:
            return 0
        assert len(matches) == 1
        return int(matches[0])

    return _extract


EXTRACT_LAYER = extract_str(r"layer-(\d+)")
EXTRACT_PARTITION = extract_str(r"partition_time-(\d+)")

class StandardStats(NamedTuple):
    log_tuple_count: int = 0
    nchunks: int = 0
    postprocess_time: float = 0.0
    setup_cost: float = 0.0
    partition_cost: float = 0.0
    notes: str = ''

    @staticmethod
    def get_sd_stats(sd_stat: dict):
        sd_standard_stats = StandardStats(
            log_tuple_count=sd_stat['tuples_count'],
            nchunks=sd_stat['nchunks'],
            postprocess_time=sd_stat['postprocess_time'],
            setup_cost=sd_stat['build_time']
        )
        return sd_standard_stats._asdict()

    @staticmethod
    def get_tp_stats(option: dict, category: str, def_partition_time=None):
        extra = None if len(option['extra']) == 0 else option['extra'][0]
        tp_stat = option['misc_key_value_layer_stats']
        tp_setup_cost = option['misc_key_value_sql_compilation_time'] / (10**6)
        if def_partition_time is None:
            partition_time=0 if extra is None else EXTRACT_PARTITION(extra)
            if partition_time != 0:
                partition_time = partition_time / (10**6)
            else:
                partition_time = 0.0
        else:
            partition_time = def_partition_time
        total_tuple_count = reduce(lambda prev, curr: prev + curr['sum'], tp_stat.values(), 0)
        nchunks = reduce(lambda prev, curr: prev + curr['count'], tp_stat.values(), 0)
        tp_standard_stats = StandardStats(
            log_tuple_count=total_tuple_count,
            nchunks=nchunks,
            setup_cost=tp_setup_cost,
            notes=category,
            partition_cost=partition_time
        )
        return tp_standard_stats._asdict()

def _handle_sd_offset(sample_inference_result: dict):
    sql_spec_map = sample_inference_result["sql_spec_map"]
    capture_indexes = [
        l_idx
        for (l_idx, (map_entry, _)) in enumerate(sql_spec_map)
        if tuple(map_entry) in (TRACEPROV_CAPTURE_ENTRY, TRACEPROV_CAPTURE_ENTRY_SD)
    ]
    last_capture_index = max(capture_indexes)
    last_capture_profile = sample_inference_result['profile'][last_capture_index]
    last_capture_time = sample_inference_result['result_time'][last_capture_index]
    offset_results = []
    result_added = set()
    for profile_entry_idx, profile_entry in enumerate(sample_inference_result['profile']):
        if profile_entry_idx in capture_indexes:
            continue
        first_key, iter_id = sql_spec_map[profile_entry_idx]
        composite_key = (tuple(first_key), iter_id)
        if composite_key in result_added:
            continue
        result_added.add(composite_key)
        # print(profile_entry)
        assert len(first_key) in (2, 3)
        if len(first_key) == 2:
            part_key = first_key
        else:
            part_key = first_key[1:]
        part_key = tuple(part_key)
        offset = part_key[-1]
        offset_results.append(
            (
                {**dict(
                phase_2=tap_simple_result(sample_inference_result['result_time'][profile_entry_idx]),
                phase_2_profile=tap_profile_result(profile_entry),
                offset=offset,
                ),
                NORM_ITER_COL: iter_id
                }
            )
        )
    other_options = dict(
        phase_1=tap_simple_result(last_capture_time),
        phase_1_profile=tap_profile_result(last_capture_profile),
        extra=StandardStats.get_sd_stats(sample_inference_result['stats'][0]),
        log_sizes=make_log_size(sample_inference_result['stats'][0]['size_mb'])
    )
    return [({**other_options, **offset_result}) for offset_result in offset_results]
        
def _handle_duckdb_tp_offset(sample_inference_result: dict, suffix):
    sql_spec_map = sample_inference_result['sql_spec_map']
    capture_indexes = [
        l_idx
        for (l_idx, (map_entry, _)) in enumerate(sql_spec_map)
        if tuple(map_entry) in (TRACEPROV_CAPTURE_ENTRY, TRACEPROV_CAPTURE_ENTRY_SD)
    ]
    last_capture_index = max(capture_indexes)
    last_capture_profile = sample_inference_result['profile'][last_capture_index]
    last_capture_time = sample_inference_result['result_time'][last_capture_index]
    last_capture_option = last_capture_time['option']
    offset_results = []
    result_added = set()
    part_times = []
    for profile_entry_idx, profile_entry in enumerate(sample_inference_result['profile']):
        if profile_entry_idx in capture_indexes:
            continue
        first_key, iter_id = sql_spec_map[profile_entry_idx]
        composite_key = (tuple(first_key), iter_id)
        if composite_key in result_added:
            continue
        result_added.add(composite_key)
        # print(profile_entry)
        assert len(first_key) == 3
        # layer_number = first_key[0]
        part_key = first_key[1:]
        part_key = tuple(part_key)
        offset = part_key[-1]
        result_time_entry = sample_inference_result['result_time'][profile_entry_idx]['option']['extra']
        assert len(result_time_entry) == 1
        layer_number = EXTRACT_LAYER(result_time_entry[0])
        partition_time = EXTRACT_PARTITION(result_time_entry[0]) / (10**6)
        part_times.append(partition_time)
        offset_results.append(
            (
                {**dict(
                phase_2=tap_simple_result(sample_inference_result['result_time'][profile_entry_idx]),
                phase_2_profile=tap_profile_result(profile_entry),
                offset=offset,
                layer_number=layer_number
                ),
                NORM_ITER_COL: iter_id
                }
            )
        )
    other_options = dict(
        phase_1=tap_simple_result(last_capture_time),
        phase_1_profile=tap_profile_result(last_capture_profile),
        log_sizes=(last_capture_option['misc_key_value_total_log_size'][0])
    )
    combined = [
        ({**other_options, **offset_result, 'extra': StandardStats.get_tp_stats(last_capture_option, suffix, partition_time)}) for partition_time,  offset_result in zip(part_times, offset_results)
    ]
    return combined

def handle_duckdb(db_system: str, mode: str, result_path: Path, rows_list: list):
    print("duckdb handling ", db_system, result_path)
    result = json_read_file(result_path)
    main_res: dict = result['results']
    terminal_name = result_path.name
    version = parse_reg(terminal_name)
    call_options = result['call_options']
    bench_parser = make_duckdb_parse()
    #add_query_options(bench_parser)
    bench_parsed, _ = bench_parser.parse_known_args(call_options)
    traceprov_handle_suffix(bench_parsed)
    thread_count = bench_parsed.threads
    main_class = TpchRow if mode == 'all' else TpchSampleRow

    def _handle_gprom_result(pack):
        query, gprom_mode, gprom_result = pack
        fail_reason = get_error(gprom_result)
        def_raw_args = dict(
            category=gprom_mode,
            query_num=query,
            parallel=thread_count,
            layer_number=0,
            version=version,
            fail_reason=fail_reason or '',
            log_sizes=None,
            phase_2=None,
            phase_2_profile=None,
            extra=None
        )
        if mode != 'all':
            def_raw_args = ({**def_raw_args, 'offset': -1})
        kwarg_list = []
        if fail_reason is None:
            for offset_id, offset_result in enumerate(gprom_result):
                infer_result = list(map(tap_profile_result, offset_result['infer']['profile']))
                time_result = list(map(tap_simple_result,  offset_result['infer']['time']))
                kwargs = dict(
                    phase_1=Extendable(time_result),
                    phase_1_profile=Extendable(infer_result),

                )
                if mode != 'all':
                    kwargs = ({**kwargs, 'offset': offset_id})
                kwarg_list.append(kwargs)
        else:
            kwarg_list.append(
                dict(
                    phase_1=None,
                    phase_1_profile=None
                )
            )
        _cls = TpchRow if mode == 'all' else TpchSampleRow
        print("Using class", _cls)
        rows = [
            _cls(**{**def_raw_args, **row_arg})
            for row_arg in kwarg_list
        ]
        return rows

    def _handle_sd_result(pack):
        query, query_result = pack
        def_raw_args = dict(
            category="smokedduck",
            query_num=query,
            parallel=thread_count,
            layer_number=0,
            version=version,
            fail_reason=""
        )
        def _add_fail(fail_reason):
            failed_dict = dict(phase_1=None, phase_1_profile=None, phase_2=None, phase_2_profile=None, log_sizes=None, extra=None, fail_reason=fail_reason)
            if mode == 'offset':
                failed_dict = ({**failed_dict, 'offset': -1})
                _cls = TpchSampleRow
            else:
                _cls = TpchRow
            failed_dict = ({**failed_dict, **def_raw_args})
            return _cls(**failed_dict)
        if mode == 'all':
            sd_result = query_result['result']['sd']
            if sd_result['capture_time'] is None:
                failed_dict = [_add_fail(DUCKDB_GENERIC)]
                return failed_dict
            phase_1_result = list(map(tap_simple_result, sd_result['capture_time']))
            phase_1_profile_result = list(map(tap_profile_result, sd_result['capture_profile']))
            phase_2_combined_result = extract_infer(sd_result['infer_results'])
            phase_2_result = list(map(tap_simple_result, phase_2_combined_result))
            phase_2_profile_result = list(map(tap_profile_result, phase_2_combined_result))
            extra_stats = list(map(StandardStats.get_sd_stats, sd_result['capture_stats']))
            log_size = list(map(lambda _stats: make_log_size(_stats['size_mb']), sd_result['capture_stats']))
            kwargs = [
                dict(
                    phase_1=Extendable(phase_1_result),
                    phase_1_profile=Extendable(phase_1_profile_result),
                    phase_2=Extendable(phase_2_result),
                    phase_2_profile=Extendable(phase_2_profile_result),
                    log_sizes=Extendable(log_size),
                    extra=Extendable(extra_stats)
                )
            ]
        else:
            sample_inference_result = query_result['sample_inference_result']
            if 'type' in sample_inference_result:
                assert 'return_code' in sample_inference_result
                rc = sample_inference_result['return_code']
                fail_reason = f"error_{rc}"
                failed_dict = [_add_fail(fail_reason)]
                return failed_dict
            kwargs = _handle_sd_offset(sample_inference_result)
            
        return list([main_class(**{**def_raw_args, **kwarg}) for kwarg in kwargs])


    def _handle_traceprov(pack):
        query, query_result = pack
        def_raw_args = dict(
            category="traceprov",
            query_num=query,
            parallel=thread_count,
            layer_number=0,
            version=version,
            fail_reason=""
        )
        suffix = DuckDBDriverOptions.get_suffix(bench_parsed)
        if mode == 'all':
            tp_result = query_result['result']
            phase_1_result = list(map(tap_simple_result, tp_result['capture_time']))
            phase_1_profile_result = list(map(tap_profile_result, tp_result['capture_profile']))
            phase_2_combined_result = extract_infer(tp_result['infer_results'])
            phase_2_result = list(map(tap_simple_result, phase_2_combined_result))
            phase_2_profile_result = list(map(tap_profile_result, phase_2_combined_result))
            extra_stats = [StandardStats.get_tp_stats(node['option'], suffix) for node in tp_result['capture_time']]
            log_size = [
                capture_time["option"]["misc_key_value_total_log_size"][0]
                for capture_time in tp_result['capture_time']
            ]
            kwargs = [
                dict(
                    phase_1=Extendable(phase_1_result),
                    phase_1_profile=Extendable(phase_1_profile_result),
                    phase_2=Extendable(phase_2_result),
                    phase_2_profile=Extendable(phase_2_profile_result),
                    log_sizes=Extendable(log_size),
                    extra=Extendable(extra_stats)
                )
            ]
        else:
            kwargs = _handle_duckdb_tp_offset(query_result['sample_inference_result'], suffix)

        return list([main_class(**{**def_raw_args, **kwarg}) for kwarg in kwargs])
    if db_system == 'gprom':
        rows = flatten([
            _handle_gprom_result((query, gprom_mode, gprom_result))
            for query, query_result in main_res.items()
            for gprom_mode, gprom_result in query_result.items()
        ])
    elif db_system == 'smokedduck':
        rows = flatten(
            _handle_sd_result((query, sd_result))
            for query, sd_result in main_res.items()
        )
    elif db_system == 'traceprov':
        rows = flatten(
            _handle_traceprov((query, tp_result))
            for query, tp_result in main_res.items()
        )
    else:
        raise Exception("Not implemented anything else!")
    rows_list.extend(rows)

class PgTimeout(Exception): ...

PG_TIMEOUT = 'timeout'
DUCKDB_SEGFAULT = 'segfault'
DUCKDB_GENERIC = 'fail_generic'
def extract_postgres_data(pg_result):
    is_timeout = pg_result.get('timeout', False)

    if is_timeout:
        raise PgTimeout()

    timing = pg_result['explain_time']
    plan = json.loads(pg_result['complete_plan'])['Plan']
    row_count = plan['Actual Rows']
    width = plan['Plan Width']
    start_cost = plan['Startup Cost']
    total_cost = plan['Total Cost']
    profile_result = make_dummy_profile_result(timing)
    gen_result = make_dummy_simple_result(timing, width, row_count)
    gen_result['start_cost'] = start_cost
    gen_result['total_cost'] = total_cost
    return profile_result, gen_result

def make_log_size(log_size):
    return dict(
        page_requested_size=log_size,
        page_used_size=log_size,
        bytes_used_size=log_size,
    )

def slice_throwaway(results):
    if len(results) == 15:
        print(f"Slicing 15->5")
        return results[5:]
    assert False, f"Not handled size: {len(results)}"

def extract_pg_mult(pg_results):
    all_profile_gen_results = [extract_postgres_data(of) for of in pg_results]
    profile_result = Extendable([t[0] for t in all_profile_gen_results])
    gen_result = Extendable([t[1] for t in all_profile_gen_results])
    return profile_result, gen_result

def extract_muller_log_size(log_size_obj):
    log_size = int(log_size_obj[0]['captured'][0][0])
    log_size_pack = make_log_size(log_size)
    return log_size_pack

def _get_muller_handle(db_system, version):
    muller_raw_args = dict(
        category=db_system,
        parallel=1,
        layer_number=0,
        version=version,
        fail_reason='',
        extra=None
    )
    def _handle_all_result_query(current_q_result):
        query_num, query_result = current_q_result
        if query_num == '15':
            return _handle_all_result_query_15(current_q_result)
        query_result = query_result['phase_1_2_combined']
        fail_reason = None
        phase_1_profile, phase_1 = (None, None)
        phase_2_profile, phase_2= (None, None)
        try:
            phase_1_profile, phase_1 = extract_pg_mult(query_result['base'])
        except PgTimeout:
            fail_reason = PG_TIMEOUT
        
        if fail_reason is None:
            try:
                phase_2_profile, phase_2 = extract_pg_mult(query_result['materialize'])
            except PgTimeout:
                fail_reason = PG_TIMEOUT
        log_sizes = Extendable([make_log_size(extra['base_muller_get_log_size']) for extra in query_result['extras']])
        kwargs = dict(
            query_num=query_num,
            phase_1=phase_1,
            phase_1_profile=phase_1_profile,
            phase_2=phase_2,
            phase_2_profile=phase_2_profile,
            log_sizes=log_sizes,
            fail_reason=fail_reason or ""
        )
        return [TpchRow(**({**muller_raw_args, **kwargs}))]
        
    def _handle_all_result_query_15(current_q_result):
        query_num, query_result = current_q_result
        extras = get_first_value(query_result)['extras']
        phase_1_results = [extra['base_phase_1_capture'][0] for extra in extras]
        phase_2_results = [extra['base_phase_2_capture'][0] for extra in extras]
        fail_reason = None
        phase_1_profile, phase_1 = (None, None)
        phase_2_profile, phase_2= (None, None)
        try:
            phase_1_profile, phase_1 = extract_pg_mult(phase_1_results)
        except PgTimeout:
            fail_reason = PG_TIMEOUT
        if fail_reason is None:
            try:
                phase_2_profile, phase_2 = extract_pg_mult(phase_2_results)
            except PgTimeout:
                fail_reason = PG_TIMEOUT
        log_sizes = Extendable([make_log_size(extra['base_muller_get_log_size']) for extra in extras])
        kwargs = dict(
            query_num=query_num,
            phase_1=phase_1,
            phase_1_profile=phase_1_profile,
            phase_2=phase_2,
            phase_2_profile=phase_2_profile,
            log_sizes=log_sizes,
            fail_reason=fail_reason or ""
        )
        return  [TpchRow(**({**muller_raw_args, **kwargs}))]

    def _handle_offset_result_query(current_q_result):
        query_num, query_result = current_q_result
        core_result = get_first_value(query_result)['extras'][-1]
        log_size_pack = extract_muller_log_size(core_result['base_muller_get_log_size'])
        phase_1_capture = core_result['base_phase_1_capture'][0]
        # phase 1 is not going to be extendable.
        phase_1_gen_result, phase_1_profile_result = (None, None)
        phase_2_gen_result, phase_2_profile_result = (None, None)
        fail_reason = None
        try:
            phase_1_profile_result, phase_1_gen_result = extract_postgres_data(phase_1_capture['timing'])
        except PgTimeout:
            fail_reason = PG_TIMEOUT
        if fail_reason is None:
            phase_2_capture = core_result['base_phase_2_capture'][0]['backtrace_results']
            offset_rows = []
            for offset_id, offset_result in enumerate(phase_2_capture):
                offset_results = slice_throwaway(offset_result['results'])
                phase_2_profile_result, phase_2_gen_result = extract_pg_mult(offset_results)
                offset_rows.append(
                    dict(
                        query_num=query_num,
                        offset=offset_id,
                        phase_1=phase_1_gen_result,
                        phase_1_profile=phase_1_profile_result,
                        phase_2=phase_2_gen_result,
                        phase_2_profile=phase_2_profile_result,
                        log_sizes=log_size_pack
                    )
                )
        else:
            return [
                TpchSampleRow(**({**muller_raw_args, **(dict(
                query_num=query_num,
                offset=-1,
                phase_1=phase_1_gen_result,
                phase_1_profile=phase_1_profile_result,
                phase_2=phase_2_gen_result,
                phase_2_profile=phase_2_profile_result,
                log_sizes=log_size_pack,
                fail_reason=fail_reason or ""
            ))}))
            ]
        return [TpchSampleRow(**({**muller_raw_args, **kwarg})) for kwarg in offset_rows]
    return _handle_all_result_query, _handle_offset_result_query


def _handle_pg_gprom(current_q_result, mode, version, thread_count):
    query_num, query_result = current_q_result
    rows = []
    _cls = TpchRow if mode == 'all' else TpchSampleRow
    for gprom_mode, gprom_result in query_result.items():
        def_raw_args = dict(
            category=gprom_mode,
            query_num=query_num,
            parallel=thread_count,
            layer_number=0,
            version=version,
            fail_reason='',
            log_sizes=None,
            phase_2=None,
            phase_2_profile=None,
            extra=None
        )
        if not isinstance(gprom_result, list):
            gprom_result = [gprom_result]
        if mode != 'all':
            def_raw_args = ({**def_raw_args, 'offset': -1})
        for offset_id, offset_result in enumerate(gprom_result):
            phase_1_profile, phase_1 = (None, None)
            fail_reason=None
            try:
                phase_1_profile, phase_1 = extract_pg_mult(offset_result['base'])
            except PgTimeout:
                fail_reason = PG_TIMEOUT
            kwargs = dict(phase_1=phase_1, phase_1_profile=phase_1_profile, fail_reason=fail_reason)
            if mode != 'all':
                kwargs = ({**kwargs, 'offset': offset_id})
            rows.append(kwargs)
    return [_cls(**({**def_raw_args, **kwarg})) for kwarg in rows]
        

def parse_postgres_parallel(extra_sql):
    WORKER_REG = r'set max_parallel_workers_per_gather=(\d+)'
    match = re.search(WORKER_REG, extra_sql)
    assert match is not None
    groups = match.groups()
    assert len(groups) == 1
    parallel = int(groups[0].strip())
    return parallel

def handle_postgres(db_system: str, mode: str, result_path: Path, rows_list: list):
    print("postgres handling ", db_system, result_path)
    result = json_read_file(result_path)
    benchmark_params_path = str(result_path.absolute()).replace("main_result.json", "benchmarks_params.json")
    benchmark_params = json_read_file(benchmark_params_path)
    extra_sql = str((benchmark_params['call_options'])).replace('\n', '')
    main_res: dict = get_first_value(result['result'])
    terminal_name = result_path.name
    version = parse_reg(terminal_name)
    parallel = parse_postgres_parallel(extra_sql)+1
    rows = []
    if db_system == 'muller':
        handler = _get_muller_handle(db_system, version)
        if mode == 'all':
            handler = handler[0]
        else:
            handler = handler[1]
        for current_result in result['result']['muller_params_default'].items():
            rows.extend(handler(current_result))
    elif db_system == 'gprom':
        for current_result in get_first_value(result['result']).items():
            rows.extend(
                _handle_pg_gprom(current_result, mode, version, parallel)
            )
    else:
        raise Exception("not implemented yet!")
    rows_list.extend(rows)

import duckdb
def dump_rows_list(sf, db_name, mode, rows_list, out_dir):
    name = f"data_{sf}_{db_name}_{mode}"
    db_name = out_dir / f"{name}.db"
    tmp_file_name = out_dir / f"{name}.json"
    normalized = [row for rows in rows_list for row in rows.normalize()]
    just_write(tmp_file_name, json.dumps(normalized))
    connection = duckdb.connect(db_name)
    cursor = connection.cursor()
    cursor.execute(
        f"create or replace table dumped as (select * from read_json_auto('{tmp_file_name.absolute()}'))"
    )
    cursor.close()
    connection.close()

def get_new_rows():
    postgres_rows = dict(offset=[], all=[])
    duckdb_rows = dict(offset=[], all=[])
    new_duckdb_rows = dict(offset=[], all=[])
    return dict(
        postgres=postgres_rows,
        duckdb=duckdb_rows,
        newduckdb=new_duckdb_rows
    )

ALL_SYSTEMS = [
    "muller",
    "gprom",
    "smokedduck",
    "traceprov",
    "provsql"
]

def safe_compare(arg_1: str | None, arg_2: str):
    return arg_1 is not None and (arg_1.lower() != arg_2.lower())

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    parser.add_argument("--out_dir", required=True)
    parser.add_argument("--db", required=False, choices=['postgres', 'duckdb', 'newduckdb'], default=None)
    parser.add_argument("--sf", required=False, choices=['sf_01', 'sf_10', 'sf_100'], default=None)
    parser.add_argument("--system", required=False, choices=ALL_SYSTEMS, default=None)
    parser.add_argument("--mode", required=False, choices=["all", "offset"], default=None)
    parsed = parser.parse_args()
    out_dir = Path(parsed.out_dir)
    os.makedirs(out_dir, exist_ok=True)
    postgres_rows = dict(offset=[], all=[])
    duckdb_rows = dict(offset=[], all=[])
    new_duckdb_rows = dict(offset=[], all=[])
    all_rows = dict()
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
            filenames = list(sorted(filenames))
            sf = adjusted_split[1]
            if sf not in all_rows:
                all_rows[sf] = get_new_rows()
            # this allows passing command lines to check if things are alright or not.
            if safe_compare(parsed.db, db_name): continue
            if safe_compare(parsed.sf, sf): continue
            if safe_compare(parsed.system, db_system): continue
            if safe_compare(parsed.mode, mode): continue
            print("Acutally using: ", adjusted)
            current_rows = all_rows[sf][db_name][mode]
            handler = handle_duckdb if (db_name == 'duckdb' or db_name == 'newduckdb') else handle_postgres
            print(handler)
            print("before length", len(current_rows))
            for file in filenames:
                if 'main_result.json' in file or 'result.json' in file:
                    handler(db_system, mode, Path(dir_path) / file , current_rows)
                    #dump_result(all_rows)
                    print("after", len(current_rows))
        else:
            ...
    dump_result(all_rows, out_dir)
    return

def dump_result(all_rows, out_dir: Path):
    for sf, sf_result in all_rows.items():
        for db, db_result in sf_result.items():
            for mode, mode_result in db_result.items():
                dump_rows_list(sf, db, mode, mode_result, out_dir)

if __name__ == '__main__':
    main()
