# just calls run_single.
from datetime import datetime
import json
import os
from pathlib import Path

# from run_single import add_query_options, run_single, json_read_file
from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse
from traceprovpy.tools.run_duckdb_generic import (
    add_query_options,
    json_read_file,
    run_single,
)

NEEDS_DISABLE = {str(2), str(11), str(15), str(17), str(18), str(20), str(22)}


def run():
    base_parser = make_duckdb_parse()
    add_query_options(base_parser)
    base_parser.add_argument("-cfg", "--config", required=True)
    base_parser.add_argument("--suff", required=True)
    parsed = base_parser.parse_args()
    config: dict = json_read_file(parsed.config)
    is_validate = config.get("validate", False)

    # for now...
    assert config["subdirs"] == ["params_default"]
    spec = json_read_file(parsed.spec)
    run_time_options = config["runTimeOptions"]
    query_configs: dict = config.get("q_config", dict())
    results = dict()
    for query in map(str, config["queries"]):
        pre_base = query_configs.get(query, dict()).get("pre_base")
        query_result = run_single(
            Path(parsed.exe),
            db=Path(parsed.db),
            query_num=query,
            base_root=Path(parsed.base_root),
            root=Path(parsed.root),
            spec_element=spec[query][0],
            use_optimized=parsed.optimized,
            validate=is_validate,
            disable_col_opt=query in NEEDS_DISABLE,
            materialize_infer=False,
            iters=run_time_options["repeat"] + run_time_options["throwaway"],
            pre_base=Path(pre_base) if pre_base is not None else None,
            run_inference=True,
        )
        assert query not in results
        results = {**results, query: dict(result=query_result)}

    current_timestamp = datetime.now()
    datetime_string = current_timestamp.strftime("%Y_%m_%d_%H_%M_%S")
    result_dir = Path(f"results/{parsed.suff}_{datetime_string}/")
    os.makedirs(result_dir, exist_ok=True)
    with open(result_dir / "result.json", "w") as f:
        f.write(json.dumps(results))


if __name__ == "__main__":
    run()
