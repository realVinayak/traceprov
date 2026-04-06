import argparse
import os
from pathlib import Path
from traceprovpy.tools.benchmark_utils import traceprov_dump_safe_results
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse
from traceprovpy.tools.run_duckdb_generic import (
    json_read_file,
    just_read,
    just_write,
    run_sample_inference,
    run_sample_inference_smokedduck,
    run_single,
    run_single_smokedduck,
)

import random

# to make sampling reproducible.
random.seed(10)


def run():
    base_parser = make_duckdb_parse()
    base_parser.add_argument("--num_groups", required=True, type=int)
    base_parser.add_argument("--config", required=True)
    parsed = base_parser.parse_args()

    result = []
    tmp = Path("./tmp/")
    os.makedirs(tmp, exist_ok=True)
    total_iters = 3
    base_sql = just_read(Path("queries/base.sql"))
    capture_sql = just_read(Path("queries/capture.sql"))
    capture_new_sql = just_read(Path("queries/capture_new.sql"))
    infer_sql = just_read(Path("queries/infer.sql"))
    validate_sql = just_read(Path("queries/validate.sql"))
    validate_new_sql = just_read(Path("queries/validate_new.sql"))
    validate_sd_sql = just_read(Path("queries/validate_sd.sql"))
    infer_offset_sql = just_read(Path("queries/infer_offset.sql"))
    validate_sd_new_sql = just_read(Path("queries/validate_new_sd.sql"))
    config = json_read_file(parsed.config)

    query = "query"
    os.makedirs(tmp / query, exist_ok=True)

    for query_dir in config:

        def run(raw_selectivity):
            selectivity = int(parsed.num_groups * (raw_selectivity / 100))
            replacer = lambda in_sql: in_sql.replace(
                "ROW_COUNT", str(query_dir["num_rows"])
            ).replace(":selectivity", str(selectivity))
            base_query = replacer(base_sql)
            just_write(tmp / query / "base.sql", base_query)
            just_write(tmp / query / "capture.sql", replacer(capture_sql))
            just_write(tmp / query / "capture_new.sql", replacer(capture_new_sql))
            just_write(tmp / query / "validate.sql", replacer(validate_sql))
            just_write(tmp / query / "validate_sd.sql", replacer(validate_sd_sql))
            just_write(tmp / query / "infer_1.sql", infer_sql)
            just_write(tmp / query / "infer_1_offset.sql", infer_offset_sql)
            just_write(tmp / query / "infer_1_new.sql", infer_sql)
            just_write(tmp / query / "infer_1_new_offset.sql", infer_offset_sql)
            just_write(
                tmp / query / "validate_new_sd.sql", replacer(validate_sd_new_sql)
            )
            just_write(tmp / query / "validate_new.sql", replacer(validate_new_sql))

            if parsed.sd_mode:
                is_new_sd = parsed.sd_mode == "new"
                query_result = dict(
                    sd_type=parsed.sd_mode,
                    sd=run_single_smokedduck(
                        Path(parsed.exe),
                        db=Path(parsed.db),
                        query_num="query",
                        root=tmp,
                        base_root=tmp,
                        iters=total_iters,
                        is_new_sd=is_new_sd,
                        sd_extension_path=(
                            None
                            if parsed.sd_extension_path is None
                            else Path(parsed.sd_extension_path)
                        ),
                        run_inference=is_new_sd,
                        validate=parsed.validate and is_new_sd,
                    ),
                )
                if parsed.sample_inference:
                    assert not is_new_sd or parsed.sample_inference == "all"
                    if not is_new_sd:
                        # normal sd case.
                        row_count = query_result["sd"]["base_time"][0]["row_count"]
                        out_ids = range(row_count)
                        if parsed.sample_inference == "sample":
                            out_ids = random.sample(out_ids, k=min(row_count, 100))
                        query_id = query_result["sd"]["capture_stats"][0]["query_id"]
                        query_result["infer_result"] = run_sample_inference_smokedduck(
                            Path(parsed.exe),
                            db=Path(parsed.db),
                            query_num=query,
                            query_id=query_id,
                            root=tmp,
                            base_root=tmp,
                            samples=out_ids,
                            iters=total_iters,
                            pre_base=None,
                            validate=parsed.validate,
                        )
                    else:
                        assert (
                            0
                        ), "didn't expect sample inference on new SD, redundant with all inference!"
            else:
                spec_element = dict(min_local_used=3, elements=[dict(idx=1)])
                query_result = dict(
                    traceprov=run_single(
                        Path(parsed.exe),
                        db=Path(parsed.db),
                        query_num="query",
                        base_root=tmp,
                        root=tmp,
                        spec_element=spec_element,
                        use_optimized=parsed.optimized,
                        validate=parsed.validate,
                        disable_col_opt=False,
                        materialize_infer=parsed.mat_infer,
                        iters=total_iters,
                        pre_base=None,
                        run_inference=True,
                    )
                )
                if parsed.sample_inference:
                    row_count = query_result["traceprov"]["base_time"][0]["row_count"]
                    out_ids = range(row_count)
                    if parsed.sample_inference == "sample":
                        out_ids = random.sample(
                            out_ids,
                            min(100, row_count),
                        )
                    elif not parsed.validate:
                        out_ids = [-1, *out_ids]

                    sample_inference_result = run_sample_inference(
                        Path(parsed.exe),
                        db=Path(parsed.db),
                        query_num=query,
                        root=tmp,
                        spec_element=spec_element,
                        samples=out_ids,
                        use_optimized=parsed.optimized,
                        iters=total_iters,
                        pre_base=None,
                        disable_col_opt=False,
                        profile=False,
                        settings=False,
                        validate=parsed.validate,
                    )
                    query_result = {
                        **query_result,
                        "sample_inference": sample_inference_result,
                    }
            result.append(
                dict(
                    dir=query_dir["num_rows"],
                    selectivity=selectivity,
                    result=query_result,
                    optimized=parsed.optimized,
                )
            )

        for sel in query_dir["sel"]:
            run(sel)

    traceprov_dump_safe_results(parsed.suff, dict(result=result))


if __name__ == "__main__":
    run()
