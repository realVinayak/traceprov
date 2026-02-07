# runs a single query (with optional validation)
# done this way to make things more manageable.


import argparse
from itertools import product
import json
import os
from pathlib import Path

from traceprovpy.tools.benchmark_utils import traceprov_assert_safe_run
from traceprovpy.tools.duckdb_inference import DuckDBDriverOptions
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse
from traceprovpy.tools.run_with_timeout import DEFAULT_REPEAT, DEFAULT_THROWAWAY

TP_OFFSET_TICKER = "__TP_OFFSET__"


def safe_file(func):
    def _func(file, *args, **kwargs):
        if file is None:
            return None
        return func(file, *args, **kwargs)

    return _func


# misc wrappers to simplify stuff.
# TODO: Put them somewhere more useful...
def just_read(file: str | Path):
    with open(file) as f:
        result = f.read()
    return result


@safe_file
def just_write(file: str | Path, contents: str):
    with open(file, "w") as f:
        f.write(contents)
    return file


@safe_file
def json_read_file(file: str):
    try:
        with open(file) as f:
            json_content = json.loads(f.read())
        return json_content
    except:
        try:
            return just_read(file)
        except:
            return None


def json_read_files(files: list[str]):
    return list(map(json_read_file, files))


@safe_file
def json_read_iters(file: str, iters: int):
    assert "%d" in file
    return [json_read_file(file.replace("%d", iter)) for iter in map(str, range(iters))]


@safe_file
def json_read_two_iters(file: str, first_iter: list[int], second_iter: list[int]):
    assert "%d_%d" in file
    return [
        json_read_file(file.replace("%d_%d", f"{a}_{b}"))
        for a, b in product(first_iter, second_iter)
    ]


def make_dump_query(in_query: str, out_path: str):
    in_query = in_query.replace(";", "")
    dump_query = f"copy (select * from ({in_query}) f order by all) to '{out_path}' (header false)"
    return dump_query


def run_sample_inference(
    exec: Path,
    db: Path,
    query_num: str,
    root: Path,
    spec_element: dict,
    samples: list[int],
    use_optimized: bool = False,
    iters: int = DEFAULT_REPEAT + DEFAULT_THROWAWAY,
    pre_base: Path | None = None,
    disable_col_opt: bool = False,
    profile: bool = True,
    settings: bool = True,
    validate: bool = False,
):

    if validate:
        iters = 1

    query_dir = root / query_num
    if use_optimized:
        captured_sql = query_dir / "capture_new.sql"
    else:
        captured_sql = query_dir / "capture.sql"
    min_layer_used = spec_element["min_local_used"]
    exec_str = exec.as_posix()
    if pre_base:
        pre_base_options = DuckDBDriverOptions(
            db=db.as_posix(), repeat=1, threads=1, i=(query_dir / pre_base).as_posix()
        )
        traceprov_assert_safe_run(f"{exec_str} {pre_base_options.serialize()}")

    sample_q_dir = Path("/tmp/")
    extra_sqls = []
    sql_spec_map = []
    for idx, element in enumerate(spec_element["elements"]):
        element_idx = element["idx"]
        if use_optimized:
            infer_path = query_dir / f"infer_{element_idx}_new_offset.sql"
        else:
            infer_path = query_dir / f"infer_{element_idx}_offset.sql"

        assert infer_path.exists()
        infer_query = just_read(infer_path)
        assert TP_OFFSET_TICKER in infer_query
        sample_element_q_dir = sample_q_dir / str(idx)
        os.makedirs(sample_element_q_dir, exist_ok=True)
        for sample_id, out_id in enumerate(samples):
            infer_with_offset = infer_query.replace(TP_OFFSET_TICKER, str(out_id))
            if validate:
                if sample_id == 0:
                    infer_with_offset = f"create or replace table LAYER_{element_idx} AS ({infer_with_offset})"
                else:
                    infer_with_offset = (
                        f"insert into LAYER_{element_idx} {infer_with_offset}"
                    )
            final_q_path = sample_element_q_dir / f"infer_{sample_id}.sql"
            just_write(final_q_path, infer_with_offset)
            extra_sqls.append(final_q_path.as_posix())
            sql_spec_map.append((element_idx, sample_id, out_id))

    just_write("/tmp/extra_file.txt", "\n".join(extra_sqls))
    sql_spec_map = product(sql_spec_map, range(iters))
    sql_spec_map = [("capture", 0), *sql_spec_map]

    capture_options = DuckDBDriverOptions(
        db=db.as_posix(),
        repeat=iters,
        threads=1,
        i=captured_sql.as_posix(),
        time="/tmp/infer_time.json",
        min_layer_number=min_layer_used,
        profile=("/tmp/infer_profile_%d_%d.json" if profile else None),
        settings=("/tmp/capture_settings.json" if settings else None),
        disable_col_opt=disable_col_opt,
        extra_file="/tmp/extra_file.txt",
        main_once_extra_all=True,
    )

    traceprov_assert_safe_run(f"{exec_str} {capture_options.serialize()}")
    capture_result_time = json_read_file(capture_options.time)
    if capture_options.profile:
        capture_profile_out = json_read_two_iters(
            capture_options.profile,
            range(1, len(extra_sqls) + 1),
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
        validate_query(query_dir, "validate.sql", capture_options.db, exec_str)
    return dict(
        result_time=capture_result_time,
        profile=capture_profile_out,
        settings=capture_settings,
        sql_spec_map=sql_spec_map,
    )


def run_single_smokedduck(
    exe: Path,
    db: Path,
    query_num: str,
    base_root: Path,
    root: Path,
    iters: int = DEFAULT_REPEAT + DEFAULT_THROWAWAY,
    pre_base: Path | None = None,
    sd_extension_path: Path = None,
    is_new_sd: bool = False,
    run_inference: bool = True,
    validate: bool = False,
    mat_infer: bool = False,
):
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
        threads=1,
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
            if mat_infer or validate:
                if mat_infer:
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


def validate_query(base_dir: Path, validate_query_name: str, db: str, exec_str: str):
    base_dump_path = Path("/tmp/") / "base_dump.csv"
    capture_dump_path = Path("/tmp/") / "capture_dump.csv"

    base_dump_query_path = Path("/tmp/") / "base_dump_query.sql"
    base_dump_query = make_dump_query(
        just_read(base_dir / "base.sql"), base_dump_path.as_posix()
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
    exec: Path,
    db: Path,
    query_num: str,
    base_root: Path,
    root: Path,
    samples: list[int],
    query_id: int,
    iters: int = DEFAULT_REPEAT + DEFAULT_THROWAWAY,
    pre_base: Path | None = None,
    profile: bool = True,
    settings: bool = True,
    validate: bool = False,
):
    sample_results = _run_sample_inference_smokedduck(
        exec,
        db,
        query_num,
        base_root,
        root,
        samples,
        query_id,
        iters,
        pre_base,
        profile,
        settings,
        validate,
    )

    if validate:
        validate_query(
            base_root / query_num, "validate_sd.sql", db.as_posix(), exec.as_posix()
        )
    return sample_results


def _run_sample_inference_smokedduck(
    exec: Path,
    db: Path,
    query_num: str,
    base_root: Path,
    root: Path,
    samples: list[int],
    query_id: int,
    iters: int = DEFAULT_REPEAT + DEFAULT_THROWAWAY,
    pre_base: Path | None = None,
    profile: bool = True,
    settings: bool = True,
    validate: bool = False,
):
    base_dir = base_root / query_num
    base_sql = base_dir / "base.sql"
    exec_str = exec.as_posix()
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

    sample_q_dir = Path("/tmp/sd_infer/") / query_num
    os.makedirs(sample_q_dir, exist_ok=True)
    extra_sqls = []
    sql_spec_map = []
    for sample_id, out_id in enumerate(samples):
        final_q_path = sample_q_dir / f"infer_{sample_id}.sql"
        infer_with_offset = f"select * from lineage_query(1, 100, {out_id}::UINTEGER)"
        if validate:
            if sample_id == 0:
                infer_with_offset = (
                    f"create or replace table LAYER_1_SD AS ({infer_with_offset})"
                )
            else:
                infer_with_offset = f"insert into LAYER_1_SD ({infer_with_offset})"

        just_write(final_q_path, infer_with_offset)
        extra_sqls.append(final_q_path.as_posix())
        sql_spec_map.append((sample_id, out_id))

    sql_spec_map = [
        ("capture", 0),
        *product(sql_spec_map, range(iters)),
    ]
    just_write("/tmp/extra_file.txt", "\n".join(extra_sqls))

    capture_options = DuckDBDriverOptions(
        db=db.as_posix(),
        repeat=iters,
        threads=1,
        i=base_sql.as_posix(),
        time="/tmp/infer_time.json",
        profile=("/tmp/infer_profile_%d_%d.json" if profile else None),
        settings=("/tmp/capture_settings.json" if settings else None),
        extra_file="/tmp/extra_file.txt",
        stats="/tmp/capture_sd_stats_%d.json",
        lineage=True,
        main_once_extra_all=True,
    )

    traceprov_assert_safe_run(f"{exec_str} {capture_options.serialize()}")
    capture_result_time = json_read_file(capture_options.time)
    if capture_options.profile:
        capture_profile_out = json_read_two_iters(
            capture_options.profile,
            range(1, len(extra_sqls) + 1),
            range(capture_options.repeat),
        )
    else:
        capture_profile_out = None
    if capture_options.settings:
        capture_settings = json_read_file(capture_options.settings)
    else:
        capture_settings = None

    assert len(capture_result_time) == len(sql_spec_map)
    return dict(
        result_time=capture_result_time,
        profile=capture_profile_out,
        settings=capture_settings,
        sql_spec_map=sql_spec_map,
    )


def extract_extras(
    time_results: list, profile_results: list, driver_options: DuckDBDriverOptions
):
    # first will be the base.
    # rest will be the extras.
    assert driver_options.extra_file is None
    assert driver_options.extra is None
    collection_size = len(driver_options.extras) + 1
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
        for extra_id in range(1, len(driver_options.extras) + 1)
    ]
    capture_result_time = [
        time_node
        for time_idx, time_node in enumerate(time_results)
        if (time_idx % collection_size) == 0
    ]
    return capture_result_time, infer_results


def run_single(
    exe: Path,
    db: Path,
    query_num: str,
    base_root: Path,
    root: Path,
    spec_element: dict,
    use_optimized: bool = False,
    validate: bool = False,
    iters: int = DEFAULT_REPEAT + DEFAULT_THROWAWAY,
    disable_col_opt: bool = False,
    materialize_infer: bool = False,
    pre_base: Path | None = None,
    run_inference: bool = True,
):
    if validate:
        materialize_infer = True
    # if validating, assert not using optimized, for now...
    # NOTE: supports the below now.
    # assert not validate or not use_optimized

    # bc those are postgres queries....
    base_sql = base_root / query_num / "base.sql"

    query_dir = root / query_num
    if use_optimized:
        captured_sql = query_dir / "capture_new.sql"
    else:
        captured_sql = query_dir / "capture.sql"

    min_layer_used = spec_element["min_local_used"]

    exec_str = exe.as_posix()

    if pre_base:
        pre_base_options = DuckDBDriverOptions(
            db=db.as_posix(), repeat=1, threads=1, i=(query_dir / pre_base).as_posix()
        )
        traceprov_assert_safe_run(f"{exec_str} {pre_base_options.serialize()}")

    base_options = DuckDBDriverOptions(
        db=db.as_posix(),
        repeat=iters,
        threads=1,
        i=base_sql.as_posix(),
        time="/tmp/base_time.json",
        min_layer_number=min_layer_used,
        profile="/tmp/base_profile_%d.json",
        settings="/tmp/base_settings.json",
    )

    traceprov_assert_safe_run(f"{exec_str} {base_options.serialize()}")
    base_result_time = json_read_file(base_options.time)
    base_profile_out = json_read_iters(base_options.profile, base_options.repeat)
    base_settings = json_read_file(base_options.settings)

    capture_options = base_options._replace(
        i=captured_sql.as_posix(),
        time="/tmp/capture_time.json",
        profile="/tmp/capture_profile_%d.json",
        disable_col_opt=disable_col_opt,
        settings="/tmp/capture_settings.json",
    )

    infer_paths = []
    if run_inference:
        for element in spec_element["elements"]:
            element_idx = element["idx"]
            if use_optimized:
                infer_path = query_dir / f"infer_{element_idx}_new.sql"
            else:
                infer_path = query_dir / f"infer_{element_idx}.sql"

            assert infer_path.exists()
            infer_path = infer_path.as_posix()
            if materialize_infer:
                contents = just_read(infer_path)
                table_name = f"LAYER_{element_idx}"
                infer_path = (
                    Path("/tmp/") / f"infer_{element_idx}_materialize.sql"
                ).as_posix()
                mat_contents = f"create or replace table {table_name} AS ({contents});"
                just_write(infer_path, mat_contents)
            infer_paths = [*infer_paths, infer_path]

    if infer_paths:
        capture_options = capture_options._replace(extras=infer_paths)

    traceprov_assert_safe_run(f"{exec_str} {capture_options.serialize()}")
    capture_result_time = json_read_file(capture_options.time)
    capture_profile_out = json_read_iters(
        capture_options.profile, capture_options.repeat
    )
    capture_settings = json_read_file(capture_options.settings)

    if infer_paths:
        capture_result_time, infer_results = extract_extras(
            capture_result_time, capture_profile_out, capture_options
        )
    else:
        infer_results = None

    if validate:
        # need to dump base and infer result on provenance, and compare both.
        validate_query(
            query_dir,
            ("validate_new.sql" if use_optimized else "validate.sql"),
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
