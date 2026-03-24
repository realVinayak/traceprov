# runs a single query (with optional validation)
# done this way to make things more manageable.


import argparse
import enum
from itertools import product
import json
import os
from pathlib import Path
import random
from typing import Iterable, Sequence, Tuple

from traceprovpy.tools.file_utils import *
from traceprovpy.tools.benchmark_utils import traceprov_assert_safe_run
from traceprovpy.tools.duckdb_inference import TRACEPROV_GRAPH_FILE, DuckDBDriverOptions
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse
from traceprovpy.tools.run_with_timeout import DEFAULT_REPEAT, DEFAULT_THROWAWAY


random.seed(10)

TP_OFFSET_TICKER = "__TP_OFFSET__"
TP_OUT_ID_TICKER = "%OUT_ID%"


def make_dump_query(in_query: str, out_path: str):
    in_query = in_query.replace(";", "")
    dump_query = f"copy (select * from ({in_query}) f order by all) to '{out_path}' (header false)"
    return dump_query

def infer_sample_id(base_row_count: int, parsed):
    out_ids = range(base_row_count)
    if parsed.sample_inference == "sample":
        out_ids = _infer_sample_id(out_ids, base_row_count, parsed.sample_num)
    return out_ids

def _infer_sample_id(out_ids: Sequence[int], row_count: int, sample_num: int):
    out_ids = random.sample(
        out_ids,
        min(sample_num, row_count),
    )
    if len(out_ids) != row_count:
        # also pick first parsed.sample_num / 4
        clamped = int(sample_num / 4)
        out_ids.extend((range(row_count))[:clamped])
        out_ids.extend((range(row_count))[-clamped:])
    return out_ids

CAPTURE_ENTRY = ("capture", 0, 0)

def create_base_offset(query_dir: Path):
    base_offset = query_dir / "base_offset.sql"
    if not base_offset.exists():
        base_query = query_dir / "base.sql"
        if base_query.exists():
            base_query_content = just_read(base_query)
            base_query_content = base_query_content.replace(";", "")
            base_query_content = f"select * from ({base_query_content}) LIMIT 1 OFFSET {TP_OFFSET_TICKER}"
            just_write(base_offset, base_query_content)
    
    assert base_offset.exists(), "Expected base offset query to exist!"

def run_sample_inference(
    query_num: str,
    samples: Iterable[int],
    parsed,
    traceprov_layers_to_derive: Tuple[int],
    iters: int = DEFAULT_REPEAT + DEFAULT_THROWAWAY,
    pre_base: Path | None = None,
    disable_col_opt: bool = False
):
    db = Path(parsed.db)
    exe = Path(parsed.exe)
    root = Path(parsed.root)
    validate = parsed.validate
    materialize_infer = parsed.mat_infer
    use_optimized=parsed.optimized
    use_aggresive_optimized=parsed.agg_optimized

    if validate:
        materialize_infer = True
        iters = 1

    query_dir = root / query_num
    captured_sql = extract_capture_query(
        query_dir,
        parsed,
        use_optimized,
        use_aggresive_optimized
    )

    exec_str = exe.as_posix()
    if pre_base:
        pre_base_options = DuckDBDriverOptions(
            db=db.as_posix(), repeat=1, threads=1, i=(query_dir / pre_base).as_posix()
        )
        traceprov_assert_safe_run(f"{exec_str} {pre_base_options.serialize()}")

    sql_spec_map = []
    
    log_offsets = []
    for sample_id, out_id in enumerate(samples):
        for element in traceprov_layers_to_derive:
            sql_spec_map.append((element, sample_id, out_id))
        log_offsets.append(out_id)

    sql_spec_map = [CAPTURE_ENTRY, *sql_spec_map]
    sql_spec_map = list(product(sql_spec_map, range(iters)))

    capture_options = DuckDBDriverOptions(
        db=db.as_posix(),
        repeat=iters,
        threads=parsed.threads,
        i=captured_sql.as_posix(),
        time="/tmp/infer_time.json",
        profile=("/tmp/infer_profile_%d_%d.json"),
        settings=("/tmp/capture_settings.json"),
        disable_col_opt=disable_col_opt,
        main_once_extra_all=True,
        log_offsets=log_offsets,
        traceprov_perform_derivation=True,
        traceprov_layers_to_derive=traceprov_layers_to_derive,
        traceprov_materialize_derivation=materialize_infer
    )

    capture_options = capture_options.parse_optimizations(parsed)

    traceprov_assert_safe_run(f"{exec_str} {capture_options.serialize()}")
    capture_result_time: list = json_read_file(capture_options.time)
    if capture_options.profile:
        capture_profile_out = json_read_two_iters(
            capture_options.profile,
            range(0, len(log_offsets) + 1),
            range(capture_options.repeat),
        )
    else:
        capture_profile_out = None
    if capture_options.settings:
        capture_settings = json_read_file(capture_options.settings)
    else:
        capture_settings = None

    assert len(capture_result_time) == len(sql_spec_map), f"Len: {capture_result_time}, {len(sql_spec_map)}"

    if validate:
        for map_idx, map_entry in enumerate(sql_spec_map):
            if map_entry == CAPTURE_ENTRY: continue
            (_, sample_id, out_id), iter_id = map_entry
            # don't do any validation in this case.
            if iter_id > 0:
                continue

            if use_optimized:
                validate_query_offset = query_dir / "validate_new_offset.sql"
            else:
                validate_query_offset = query_dir / "validate_offset.sql"

            validate_out = query_dir / "replaced_validate.tmp.sql"
            base_out = query_dir / "replaced_base.tmp.sql"

            query_str = just_read(validate_query_offset)
            query_str = query_str.replace(TP_OUT_ID_TICKER, str(out_id))
            query_str = query_str.replace("LAYER", "traceprov_lineage")

            just_write(validate_out, query_str)

            base_offset = query_dir / "base_offset.sql"
            just_write(
                base_out,
                just_read(base_offset).replace(TP_OFFSET_TICKER, str(out_id)),
            )

            validate_query(
                query_dir,
                validate_out.parts[-1],
                capture_options.db,
                exec_str,
                base_out.parts[-1],
            )

    return dict(
        result_time=capture_result_time,
        profile=capture_profile_out,
        settings=capture_settings,
        sql_spec_map=sql_spec_map,
    )

def get_is_new_sd(parsed):
    return parsed.sd_mode == "new"

def run_single_smokedduck(
    query_num: str,
    parsed,
    iters: int = DEFAULT_REPEAT + DEFAULT_THROWAWAY,
    pre_base: Path | None = None
):

    db = Path(parsed.db)
    exe = Path(parsed.exe)
    base_root = Path(parsed.base_root)
    root = Path(parsed.root)
    validate = parsed.validate
    materialize_infer = parsed.mat_infer
    run_inference = parsed.infer and parsed.sample_inference is None
    is_new_sd = get_is_new_sd(parsed)
    run_sd=parsed.sample_inference is None

    sd_extension_path=(
        None
        if parsed.sd_extension_path is None
        else Path(parsed.sd_extension_path)
    ),

    base_dir = base_root / query_num
    base_sql = base_dir / "base.sql"
    exec_str = exe.as_posix()
    if pre_base:
        pre_base_options = DuckDBDriverOptions(
            db=db.as_posix(),
            repeat=1,
            threads=1,
            i=(root / query_num / pre_base).as_posix(),
        )
        traceprov_assert_safe_run(f"{exec_str} {pre_base_options.serialize()}")

    base_options = DuckDBDriverOptions(
        db=db.as_posix(),
        repeat=iters,
        threads=parsed.threads,
        i=base_sql.as_posix(),
        time="/tmp/base_time.json",
        profile="/tmp/base_profile_%d.json",
        settings="/tmp/base_settings.json",
    )
    traceprov_assert_safe_run(f"{exec_str} {base_options.serialize()}")
    base_result_time = json_read_file(base_options.time)
    base_profile_out = json_read_iters(base_options.profile, base_options.repeat)
    base_settings = json_read_file(base_options.settings)

    capture_options = base_options._replace(
        time="/tmp/capture_time.json",
        profile="/tmp/capture_profile_%d.json",
        settings="/tmp/capture_settings.json",
        stats="/tmp/capture_sd_stats_%d.json",
        lineage=True,
        is_new_sd=is_new_sd,
        sd_extension_path=sd_extension_path,
    )

    extras = []
    if run_inference:
        if is_new_sd:
            infer_sql = "select * from read_block(0)"
            if materialize_infer or validate:
                if materialize_infer:
                    # so that it's easier to differniate them :)
                    table = f"LAYER_1_{query_num}_new_sd"
                else:
                    table = f"LAYER_1"
                infer_sql = f"create or replace table {table} AS ({infer_sql})"

            extras = [
                just_write("/tmp/prepare.sql", "PRAGMA PrepareLineage(0);"),
                just_write("/tmp/run_infer.sql", infer_sql),
            ]
        else:
            assert 0, "not supported yet, use sample inference branch"
    
    if run_sd:
        capture_options = capture_options._replace(extras=extras)
        traceprov_assert_safe_run(f"{exec_str} {capture_options.serialize()}")
        capture_result_time = json_read_file(capture_options.time)
        capture_profile_out = json_read_iters(
            capture_options.profile, capture_options.repeat
        )
        capture_settings = json_read_file(capture_options.settings)
        capture_stats = json_read_iters(capture_options.stats, capture_options.repeat)

        if validate:
            if is_new_sd:
                validate_query(
                    root / query_num, "validate_new_sd.sql", capture_options.db, exec_str
                )
            else:
                assert 0, "no validaton support in this call path for old smokedduck"

        if run_inference:
            capture_result_time, infer_results = extract_extras(
                capture_result_time, capture_profile_out, capture_options
            )
        else:
            infer_results = None
    else:
        capture_result_time = None
        capture_profile_out = None
        infer_results = None
        capture_settings = None
        capture_stats = None

    final_result = dict(
        base_time=base_result_time,
        base_profile=base_profile_out,
        capture_time=capture_result_time,
        capture_profile=capture_profile_out,
        base_settings=base_settings,
        capture_settings=capture_settings,
        capture_stats=capture_stats,
        infer_results=infer_results,
    )
    return final_result


def validate_query(
    base_dir: Path,
    validate_query_name: str,
    db: str,
    exec_str: str,
    base_query_name="base.sql",
):
    base_dump_path = Path("/tmp/") / "base_dump.csv"
    capture_dump_path = Path("/tmp/") / "capture_dump.csv"

    base_dump_query_path = Path("/tmp/") / "base_dump_query.sql"
    base_dump_query = make_dump_query(
        just_read(base_dir / base_query_name), base_dump_path.as_posix()
    )
    just_write(base_dump_query_path, base_dump_query)

    validate_query = base_dir / validate_query_name
    capture_dump_query_path = Path("/tmp/") / "capture_dump_query.sql"
    capture_dump_query = make_dump_query(
        just_read(validate_query), capture_dump_path.as_posix()
    )
    just_write(capture_dump_query_path, capture_dump_query)
    simple_option = lambda in_path: DuckDBDriverOptions(
        db=db,
        i=in_path.as_posix(),
        repeat=1,
        threads=12
    )
    traceprov_assert_safe_run(
        f"{exec_str} {simple_option(base_dump_query_path).serialize()}"
    )
    traceprov_assert_safe_run(
        f"{exec_str} {simple_option(capture_dump_query_path).serialize()}"
    )

    print(base_dump_path, base_dump_path.stat().st_size)
    print(capture_dump_path, capture_dump_path.stat().st_size)

    traceprov_assert_safe_run(
        f"diff {base_dump_path.as_posix()} {capture_dump_path.as_posix()}"
    )

def run_sample_inference_smokedduck(
    query_num: str,
    samples: list[int],
    query_id: int,
    parsed,
    iters: int = DEFAULT_REPEAT + DEFAULT_THROWAWAY,
    pre_base: Path | None = None
):
    
    db = Path(parsed.db)
    exe = Path(parsed.exe)
    base_root = Path(parsed.base_root)
    root = Path(parsed.root)
    validate = parsed.validate
    materialize_infer = parsed.mat_infer

    base_dir = base_root / query_num
    query_dir = root / query_num
    base_sql = base_dir / "base.sql"
    exec_str = exe.as_posix()
    if validate:
        iters = 1
    if pre_base:
        pre_base_options = DuckDBDriverOptions(
            db=db.as_posix(),
            repeat=1,
            threads=1,
            i=(root / query_num / pre_base).as_posix(),
        )
        traceprov_assert_safe_run(f"{exec_str} {pre_base_options.serialize()}")

    sample_q_dir = Path("./tmp/sd_infer/") / query_num
    os.makedirs(sample_q_dir, exist_ok=True)
    extra_sqls = []
    sql_spec_map = []
    for sample_id, out_id in enumerate(samples):
        final_q_path = sample_q_dir / f"infer_{sample_id}.sql"
        infer_with_offset = f"select * from lineage_query({query_id}, 100, {out_id}::UINTEGER)"
        if validate or materialize_infer:
            infer_with_offset = (
                f"create or replace table LAYER_1_SD_{out_id} AS ({infer_with_offset})"
            )
        just_write(final_q_path, infer_with_offset)
        extra_sqls.append(final_q_path.as_posix())
        sql_spec_map.append((sample_id, out_id))

    sql_spec_map = [('capture', 0), *sql_spec_map]
    sql_spec_map = list(product(sql_spec_map, range(iters)))
    just_write("./tmp/extra_file.txt", "\n".join(extra_sqls))

    capture_options = DuckDBDriverOptions(
        db=db.as_posix(),
        repeat=iters,
        threads=parsed.threads,
        i=base_sql.as_posix(),
        time="./tmp/infer_time.json",
        profile=("./tmp/infer_profile_%d_%d.json"),
        settings=("./tmp/capture_settings.json"),
        extra_file="./tmp/extra_file.txt",
        stats="./tmp/capture_sd_stats_%d.json",
        lineage=True,
        main_once_extra_all=True,
    )

    traceprov_assert_safe_run(f"{exec_str} {capture_options.serialize()}")
    capture_result_time = json_read_file(capture_options.time)
    if capture_options.profile:
        capture_profile_out = json_read_two_iters(
            capture_options.profile,
            range(0, len(extra_sqls) + 1),
            range(capture_options.repeat),
        )
    else:
        capture_profile_out = None
    if capture_options.settings:
        capture_settings = json_read_file(capture_options.settings)
    else:
        capture_settings = None

    assert len(capture_result_time) == len(sql_spec_map)

    if validate:
        for map_idx, map_entry in enumerate(sql_spec_map):
            if map_idx == 0:
                continue
            (sample_id, out_id), iter_id = map_entry
            if iter_id > 0:
                continue
            validate_out = query_dir / "replaced_validate.tmp.sql"
            base_out = query_dir / "replaced_base.tmp.sql"

            validate_query_offset = query_dir / "validate_sd.sql"
            query_str = just_read(validate_query_offset)
            assert TP_OUT_ID_TICKER in query_str
            query_str = query_str.replace(TP_OUT_ID_TICKER, str(out_id))
            just_write(validate_out, query_str)
            base_offset = query_dir / "base_offset.sql"
            just_write(
                base_out,
                just_read(base_offset).replace(TP_OFFSET_TICKER, str(out_id)),
            )
            validate_query(
                query_dir,
                validate_out.parts[-1],
                capture_options.db,
                exec_str,
                base_out.parts[-1],
            )


    return dict(
        result_time=capture_result_time,
        profile=capture_profile_out,
        settings=capture_settings,
        sql_spec_map=sql_spec_map,
    )


def extract_extras(
    time_results: list, profile_results: list, driver_options: DuckDBDriverOptions, extra_count: int
):
    # first will be the base.
    # rest will be the extras.
    assert driver_options.extra_file is None
    assert driver_options.extra is None
    collection_size = extra_count + 1
    assert len(time_results) == (collection_size * driver_options.repeat)
    # this should be correctly sized (just be the base repeat)
    assert len(profile_results) == driver_options.repeat
    infer_results = [
        dict(
            infer_id=extra_id,
            profile=json_read_files(
                f'{driver_options.profile.replace("%d", str(iter_id))}_{extra_id}_extra.json'
                for iter_id in (range(driver_options.repeat))
            ),
            times=[
                time_node
                for time_idx, time_node in enumerate(time_results)
                if (time_idx % collection_size) == extra_id
            ],
        )
        for extra_id in range(1, extra_count + 1)
    ]
    capture_result_time = [
        time_node
        for time_idx, time_node in enumerate(time_results)
        if (time_idx % collection_size) == 0
    ]
    return capture_result_time, infer_results

def extract_capture_query(query_dir: Path, parsed, use_optimized: bool, use_aggresive_optimized: bool):
    traceprov_use_compact = parsed.traceprov_use_compact
    if use_optimized:
        captured_sql = None
        if use_aggresive_optimized:
            captured_sql = query_dir / "capture_ignore_gn.sql"
            if not captured_sql.exists():
                assert False, "Expected ignore to be set!"
                captured_sql = None
        if traceprov_use_compact:
            captured_sql = query_dir / "capture_new_compact.sql"
            assert captured_sql.exists(), "Expected compact to be set!"
        if captured_sql is None:
            captured_sql = query_dir / "capture_new.sql"
    else:
        captured_sql = query_dir / "capture.sql"

    assert captured_sql is not None
    assert captured_sql.exists(), f"Expected capture query to exist: {captured_sql.as_posix()}"
    return captured_sql


def extract_graph_dir(parsed):
    graph_dir = Path(parsed.graph_dir)
    if parsed.optimized:
        graph_dir = graph_dir / "optimized"
    else:
        graph_dir = graph_dir / "non_optimized"
    assert graph_dir.exists()
    return graph_dir

def run_single(
    query_num: str,
    traceprov_graph_path: Path,
    traceprov_layers_to_derive: Tuple[int],
    parsed,
    iters: int = DEFAULT_REPEAT + DEFAULT_THROWAWAY,
    disable_col_opt: bool = False,
    pre_base: Path | None = None
):
    
    db = Path(parsed.db)
    exe = Path(parsed.exe)
    base_root = Path(parsed.base_root)
    root = Path(parsed.root)
    validate = parsed.validate
    materialize_infer = parsed.mat_infer
    run_inference = parsed.infer and parsed.sample_inference is None
    pending = parsed.pending
    use_optimized=parsed.optimized
    use_aggresive_optimized=parsed.agg_optimized

    if validate:
        materialize_infer = True
    # if validating, assert not using optimized, for now...
    # NOTE: supports the below now.
    # assert not validate or not use_optimized

    # bc those are postgres queries....
    base_sql = base_root / query_num / "base.sql"

    query_dir = root / query_num

    captured_sql = extract_capture_query(query_dir, parsed, use_optimized, use_aggresive_optimized)

    exec_str = exe.as_posix()

    if pre_base:
        pre_base_options = DuckDBDriverOptions(
            db=db.as_posix(), repeat=1, threads=1, i=(query_dir / pre_base).as_posix()
        )
        traceprov_assert_safe_run(f"{exec_str} {pre_base_options.serialize()}")

    threads = parsed.threads
    base_options = DuckDBDriverOptions(
        db=db.as_posix(),
        repeat=iters,
        threads=threads,
        i=base_sql.as_posix(),
        time="./tmp/base_time.json",
        profile="./tmp/base_profile_%d.json",
        settings="./tmp/base_settings.json",
        pending=pending
    )

    traceprov_assert_safe_run(f"{exec_str} {base_options.serialize()}")
    base_result_time = json_read_file(base_options.time)
    base_profile_out = json_read_iters(base_options.profile, base_options.repeat)
    base_settings = json_read_file(base_options.settings)

    capture_options = base_options._replace(
        i=captured_sql.as_posix(),
        time="./tmp/capture_time.json",
        profile="./tmp/capture_profile_%d.json",
        disable_col_opt=disable_col_opt,
        settings="./tmp/capture_settings.json",
        traceprov_materialize_derivation=(validate or materialize_infer) and run_inference,
        traceprov_layers_to_derive=traceprov_layers_to_derive
    ).parse_optimizations(parsed)

    graph_file_dest = Path(TRACEPROV_GRAPH_FILE).parent
    os.makedirs(graph_file_dest, exist_ok=True)
    traceprov_assert_safe_run(f"cp {traceprov_graph_path} {TRACEPROV_GRAPH_FILE}")

    if run_inference:
        capture_options = capture_options._replace(
            traceprov_perform_derivation=True,
            traceprov_dry_run_derivation=parsed.traceprov_dry_run_derivation
        )

    traceprov_assert_safe_run(f"{exec_str} {capture_options.serialize()}")
    capture_result_time = json_read_file(capture_options.time)
    capture_profile_out = json_read_iters(
        capture_options.profile, capture_options.repeat
    )
    capture_settings = json_read_file(capture_options.settings)

    if run_inference:
        collection_size = int(len(capture_result_time) / (capture_options.repeat)) - 1
        # print(capture_options)
        capture_result_time, infer_results = extract_extras(
            capture_result_time, capture_profile_out, capture_options, collection_size
        )
    else:
        infer_results = None

    if validate and run_inference:
        # need to dump base and infer result on provenance, and compare both.
        if use_optimized:
            validate_path = None
            if use_aggresive_optimized:
                if (query_dir / "validate_new_ignore_gn.sql").exists():
                    validate_path = "validate_new_ignore_gn.sql"
                else:
                    assert False, "Expected ignore to be set!"
            if validate_path is None:
                validate_path = "validate_new.sql"
        else:
            validate_path = "validate.sql"
        validate_query(
            query_dir,
            validate_path,
            base_options.db,
            exec_str,
        )

    final_result = dict(
        base_time=base_result_time,
        base_profile=base_profile_out,
        capture_time=capture_result_time,
        capture_profile=capture_profile_out,
        infer_results=infer_results,
        base_settings=base_settings,
        capture_settings=capture_settings,
    )
    return final_result

def run_combined(parsed, total_iters, query):
    sample_inference_result = None
    if parsed.sd_mode:
        query_result = dict(
            sd_type=parsed.sd_mode,
            sd=run_single_smokedduck(
                query_num=query,
                parsed=parsed,
                iters=total_iters,
                pre_base=None
            ),
        )
    else:
        graph_dir = Path(parsed.graph_dir)
        query_result = run_single(
            query_num=query,
            traceprov_graph_path=graph_dir / query / "graph.bin",
            traceprov_layers_to_derive=(1,),
            parsed=parsed,
            iters=total_iters,
            pre_base=None
        )
    
    if parsed.sample_inference:
        # need to sample the inference.
        if parsed.sd_mode:
            base_result = query_result["sd"]["base_time"][0]
        else:
            base_result = query_result["base_time"][0]
        base_row_count: int = base_result["row_count"]
        out_ids = infer_sample_id(base_row_count, parsed)

        if parsed.sd_mode:
            query_id = 4
            sample_inference_result = run_sample_inference_smokedduck(
                query_num=query,
                samples=out_ids,
                query_id=query_id,
                parsed=parsed,
                iters=total_iters,
            )
        else:
            sample_inference_result = run_sample_inference(
                query_num=query,
                samples=out_ids,
                parsed=parsed,
                traceprov_layers_to_derive=(1,),
                iters=total_iters
            )
    query_result = {
        **query_result,
        "sample_inference": sample_inference_result,
    }
    return query_result


def add_query_options(parser: argparse.ArgumentParser):
    parser.add_argument("--spec", required=True)
    parser.add_argument("--base_root", required=True)
    parser.add_argument("--root", required=True)


def main():
    base_parser = make_duckdb_parse()
    base_parser.add_argument("--qnum", required=True)
    add_query_options(base_parser)
    base_parser.add_argument(
        "--validate", action=argparse.BooleanOptionalAction, default=False
    )
    base_parser.add_argument(
        "--mat_infer", action=argparse.BooleanOptionalAction, default=False
    )
    base_parser.add_argument(
        "--disable_duck_opt", action=argparse.BooleanOptionalAction, default=False
    )
    # Stuff to run just before the base..
    # Maybe handle this more nicely (as done in Postgres implementation)
    base_parser.add_argument("--pre_base", required=False)
    base_parser.add_argument(
        "--run_infer", action=argparse.BooleanOptionalAction, default=True
    )
    base_parser.add_argument("--out_path", required=True)
    parsed = base_parser.parse_args()
    result = run_single(
        exe=Path(parsed.exe),
        db=Path(parsed.db),
        query_num=parsed.qnum,
        base_root=Path(parsed.base_root),
        root=Path(parsed.root),
        spec_element=json_read_file(parsed.spec)[parsed.qnum][0],
        use_optimized=parsed.optimized,
        validate=parsed.validate,
        disable_col_opt=parsed.disable_duck_opt,
        materialize_infer=parsed.mat_infer,
        # temp...
        iters=3,
        pre_base=Path(parsed.pre_base) if parsed.pre_base is not None else None,
        run_inference=parsed.run_infer,
    )
    out_path = Path(parsed.out_path)
    os.makedirs(out_path.parent, exist_ok=True)
    with open(out_path, "w") as f:
        f.write(json.dumps(result, indent=4))


if __name__ == "__main__":
    main()
