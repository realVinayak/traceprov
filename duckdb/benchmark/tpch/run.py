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
    extract_graph_dir,
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
    base_parser.add_argument("--query_layer_cfg", required=True)

    parsed = base_parser.parse_args()
    config: dict = json_read_file(parsed.config)
    query_layer_config: dict = json_read_file(parsed.query_layer_cfg)

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
            query_result = dict(
                sd_type=parsed.sd_mode,
                sd=run_single_smokedduck(
                    query_num=query,
                    parsed=parsed,
                    iters=total_iters,
                    pre_base=pre_base_path,
                ),
            )
        else:
            disable_col_opt = query in NEEDS_DISABLE
            graph_dir = extract_graph_dir(parsed)
            query_result = run_single(
                query_num=query,
                traceprov_graph_path=graph_dir / query / "graph.bin",
                traceprov_layers_to_derive=tuple(
                    query_layer_config[query]["layers_used"]
                ),
                parsed=parsed,
                iters=total_iters,
                disable_col_opt=disable_col_opt,
                pre_base=pre_base_path,
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
                    pre_base=pre_base_path,
                )
            else:
                sample_inference_result = run_sample_inference(
                    query_num=query,
                    samples=out_ids,
                    parsed=parsed,
                    iters=total_iters,
                    pre_base=pre_base_path,
                    disable_col_opt=disable_col_opt,
                    traceprov_layers_to_derive=tuple(
                        query_layer_config[query]["layers_used"]
                    ),
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
