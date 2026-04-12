import argparse

from traceprovpy.tools.benchmark import (
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
from traceprovpy.tools.benchmark_utils import TRACEPROV_MAKE_TRUNCATE_LOGS
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.run_with_timeout import (
    ReplaceSelectivity,
    RunParams,
)


def get_init(num_row: int):
    drop_view = f"DROP MATERIALIZED VIEW IF EXISTS skew_1_0_num_{num_row}_1 CASCADE;"
    drop_view_row = (
        f"DROP MATERIALIZED VIEW IF EXISTS skew_1_0_num_{num_row}_2_row CASCADE;"
    )
    create_view = f"""
        CREATE MATERIALIZED VIEW skew_1_0_num_{num_row}_1 AS 
        (SELECT (nextval('prov_id'))::int4 AS tuid,id,z,val 
        FROM skew_1_0_num_{num_row} AS skew_1_0_num_{num_row}(id,z,val));
    """
    create_view_row = f"""
    CREATE MATERIALIZED VIEW skew_1_0_num_{num_row}_2_row AS
    (SELECT "skew_1_0_num_{num_row}_1"."tuid" AS "tuid",
            ARRAY[(nextval('prov_id')) :: int4] :: int4[] AS "id"
    FROM skew_1_0_num_{num_row}_1);
    """
    return [drop_view, drop_view_row, create_view, create_view_row]


def get_query(query_name: str, num_rows: list[int], materialize: bool):
    def _get_query(num_row: int):
        replacer = ReplaceSelectivity(num_row, clause="NUM")
        base_query = Query(
            query_name=query_name,
            spec=QuerySpec(
                base="base.sql", key=f"base_{num_row}", preprocess=[replacer]
            ),
        )
        muller_query = Query(
            query_name=query_name,
            spec=QuerySpec(
                base="muller_phase_1.sql",
                materialize="muller_phase_2.sql",
                key=f"muller_{num_row}",
                preprocess=[replacer],
                extras=[TRACEPROV_MAKE_TRUNCATE_LOGS()],
            ),
        )
        return [base_query, muller_query]

    return [
        single_query for num_row in num_rows for single_query in _get_query(num_row)
    ]


def main():
    benchmark = GenericBenchmark("muller-window")
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", required=True)
    parser.add_argument("--mat", action=argparse.BooleanOptionalAction, default=False)
    parsed, _ = parser.parse_known_args()
    config = json_read_file(parsed.config)
    assert config is not None

    dir_queries = []
    num_rows = config["num_rows"]
    for subdir in config["queries"]:
        dir_queries = [*dir_queries, *get_query(subdir, num_rows, parsed.mat)]
    query_dir = QueryDirectory(dir_name="queries", queries=dir_queries)
    init_sqls = [query for num_row in num_rows for query in get_init(num_row)]
    result = benchmark.run_from_argparse(
        [query_dir],
        RunParams(**config.get("runTimeOptions", {})),
        init_sql=["select truncateLogs();", *init_sqls],
    )
    result["extras"] = dict(config=parsed.config)
    benchmark.dump_final_result(result)


if __name__ == "__main__":
    main()
