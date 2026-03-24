# just calls run_single.
import argparse
from datetime import datetime
from email.mime import base
from importlib.machinery import PathFinder
import json
import os
from pathlib import Path
from pickletools import optimize

# from run_single import add_query_options, run_single, json_read_file
from traceprovpy.tools.benchmark_utils import traceprov_dump_safe_results
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse
from traceprovpy.tools.run_duckdb_generic import (
    add_query_options,
    infer_sample_id,
    json_read_file,
    run_sample_inference,
    run_sample_inference_smokedduck,
    run_single,
    run_single_smokedduck,
)

NEEDS_DISABLE = {str(2), str(11), str(15), str(17), str(18), str(20), str(22)}

MAX_SAMPLE_COUNT = 100


def run():
    base_parser = make_duckdb_parse()
    add_query_options(base_parser)
    base_parser.add_argument("-cfg", "--config", required=True)
    base_parser.add_argument("-part_cfg", required=True)
    base_parser.add_argument("--query_layer_cfg", required=True)

    parsed = base_parser.parse_args()
    config: dict = json_read_file(parsed.config)
    part_config: dict = json_read_file(parsed.part_cfg)
    query_layer_config: dict = json_read_file(parsed.query_layer_cfg)
    is_validate = config.get("validate", False) or parsed.validate

    # for now...
    assert config["subdirs"] == ["params_default"]
    spec = json_read_file(parsed.spec)
    run_time_options = config["runTimeOptions"]
    query_configs: dict = config.get("q_config", dict())
    results = dict()
    total_iters = run_time_options["repeat"] + run_time_options["throwaway"]
    config_queries = config["queries"]
    if isinstance(config_queries, str):
        config_queries = eval(config_queries)
    for query in map(str, config_queries):
        pre_base = query_configs.get(query, dict()).get("pre_base")
        pre_base_path = Path(pre_base) if pre_base is not None else None
        sample_inference_result = None
        if parsed.sd_mode:
            is_new_sd = parsed.sd_mode == "new"
            query_result = dict(
                sd_type=parsed.sd_mode,
                sd=run_single_smokedduck(
                    exe=Path(parsed.exe),
                    db=Path(parsed.db),
                    query_num=query,
                    base_root=Path(parsed.base_root),
                    root=Path(parsed.root),
                    iters=total_iters,
                    pre_base=pre_base_path,
                    is_new_sd=is_new_sd,
                    sd_extension_path=(
                        None
                        if parsed.sd_extension_path is None
                        else Path(parsed.sd_extension_path)
                    ),
                    run_inference=is_new_sd and parsed.infer,
                    validate=parsed.validate and is_new_sd,
                    mat_infer=parsed.mat_infer,
                    run_sd=parsed.sample_inference is None,
                    parsed=parsed
                ),
            )
        else:
            disable_col_opt = query in NEEDS_DISABLE
            graph_dir = Path(parsed.graph_dir)
            if parsed.optimized:
                graph_dir = graph_dir / "optimized"
            else:
                graph_dir = graph_dir / "non_optimized"
            query_result = run_single(
                exe=Path(parsed.exe),
                db=Path(parsed.db),
                query_num=query,
                base_root=Path(parsed.base_root),
                root=Path(parsed.root),
                traceprov_graph_path=graph_dir / query / "graph.bin",
                threads=parsed.threads,
                spec=spec[query][0],
                traceprov_layers_to_derive=tuple(query_layer_config[query]["layers_used"]),
                use_optimized=parsed.optimized,
                use_aggresive_optimized=parsed.agg_optimized,
                validate=is_validate and parsed.sample_inference is None,
                disable_col_opt=disable_col_opt,
                materialize_infer=parsed.mat_infer,
                iters=total_iters,
                pre_base=pre_base_path,
                run_inference=parsed.infer and parsed.sample_inference is None,
                strict=parsed.strict,
                pending=parsed.pending,
                parsed=parsed
            )

        if parsed.sample_inference:
            # need to sample the inference.
            if parsed.sd_mode:
                base_result = query_result["sd"]["base_time"][0]
            else:
                base_result = query_result["base_time"][0]
            base_row_count: int = base_result["row_count"]
            out_ids = range(base_row_count)
            if parsed.sample_inference == "sample":
                out_ids = infer_sample_id(out_ids, base_row_count, parsed.sample_num)

            if parsed.sd_mode:
                query_id = 4
                sample_inference_result = run_sample_inference_smokedduck(
                    Path(parsed.exe),
                    db=Path(parsed.db),
                    query_num=query,
                    query_id=query_id,
                    root=Path(parsed.root),
                    base_root=Path(parsed.base_root),
                    samples=out_ids,
                    iters=total_iters,
                    pre_base=pre_base_path,
                    mat_infer=parsed.mat_infer,
                    validate=parsed.validate,
                    parsed=parsed
                )
            else:
                if parsed.sample_inference != "sample":
                    out_ids = [-1, *out_ids]
                sample_inference_result = run_sample_inference(
                    Path(parsed.exe),
                    db=Path(parsed.db),
                    query_num=query,
                    root=Path(parsed.root),
                    spec_element=spec[query][0],
                    samples=out_ids,
                    use_optimized=parsed.optimized,
                    iters=total_iters,
                    pre_base=pre_base_path,
                    disable_col_opt=disable_col_opt,
                    profile=True,
                    settings=False,
                    validate=parsed.validate,
                    mat_infer=parsed.mat_infer,
                    table_suff=parsed.suff,
                    parsed=parsed,
                    use_aggresive_optimized=parsed.agg_optimized
                )

        assert query not in results
        results = {
            **results,
            query: dict(
                result=query_result, sample_inference_result=sample_inference_result
            ),
        }

    traceprov_dump_safe_results(parsed.suff, results)


if __name__ == "__main__":
    run()
