import argparse
import os
from pathlib import Path
from traceprovpy.tools.benchmark_utils import traceprov_dump_safe_results
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse
from traceprovpy.tools.run_duckdb_generic import (
    infer_sample_id,
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
    base_parser.add_argument(
        "--random", action=argparse.BooleanOptionalAction, default=True
    )
    parsed = base_parser.parse_args()

    result = []
    tmp = Path("./tmp/")
    os.makedirs(tmp, exist_ok=True)
    total_iters = 10
    base_sql = just_read(Path("queries/base.sql"))
    base_offset_sql = just_read(Path("queries/base_offset.sql"))
    capture_sql = just_read(Path("queries/capture.sql"))
    capture_new_sql = just_read(Path("queries/capture_new.sql"))
    capture_new_compact_sql = just_read(Path("queries/capture_new_compact.sql"))
    validate_sql = just_read(Path("queries/validate.sql"))
    validate_new_sql = just_read(Path("queries/validate_new.sql"))
    validate_offset_sql = just_read(Path("queries/validate_offset.sql"))
    validate_new_offset_sql = just_read(Path("queries/validate_new_offset.sql"))
    validate_sd_sql = just_read(Path("queries/validate_sd.sql"))
    validate_sd_new_sql = just_read(Path("queries/validate_new_sd.sql"))
    config = json_read_file(parsed.config)
    assert config is not None

    query = "query"
    os.makedirs(tmp / query, exist_ok=True)
    parsed.root = tmp.as_posix()
    parsed.base_root = tmp.as_posix()

    for query_dir in config:

        def run(raw_selectivity):
            selectivity = int(parsed.num_groups * (raw_selectivity / 100))
            def replacer(in_sql: str):
                query_str = in_sql.replace("ROW_COUNT", str(query_dir["num_rows"]))
                query_str = query_str.replace(":selectivity", str(selectivity))
                if not parsed.random:
                    query_str = query_str.replace("_random", "")
                return query_str

            just_write(tmp / query / "base.sql", replacer(base_sql))
            just_write(tmp / query / "base_offset.sql", replacer(base_offset_sql))
            just_write(tmp / query / "capture.sql", replacer(capture_sql))
            just_write(tmp / query / "capture_new.sql", replacer(capture_new_sql))
            just_write(tmp / query / "capture_new_compact.sql", replacer(capture_new_compact_sql))
            just_write(tmp / query / "validate.sql", replacer(validate_sql))
            just_write(tmp / query / "validate_sd.sql", replacer(validate_sd_sql))
            just_write(
                tmp / query / "validate_offset.sql", replacer(validate_offset_sql)
            )
            just_write(
                tmp / query / "validate_new_offset.sql",
                replacer(validate_new_offset_sql),
            )
            just_write(
                tmp / query / "validate_new_sd.sql", replacer(validate_sd_new_sql)
            )
            just_write(tmp / query / "validate_new.sql", replacer(validate_new_sql))

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
