from pathlib import Path
from typing import NamedTuple

from traceprovpy.tools.extract_query_results import (
    _get_muller_handle,
    _get_pg_traceprov_handle,
    _get_provsql_handle,
    dump_rows_list,
    handle_duckdb_result,
)
from traceprovpy.tools.file_utils import json_read_file, set_query_num
from traceprovpy.tools.normalized_row import ResultExtractor


class MockKeyLog(NamedTuple):
    num_rows: int
    col_count: int

    def get_query_id():
        return ("query_num_num_rows", "query_num_col_count")


def make_row_column(row_count_str: str, column_key: str):
    assert "row_" in row_count_str
    row_count = int(row_count_str.replace("row_", ""))
    column_count = int(column_key.replace("col_", ""))
    return MockKeyLog(num_rows=row_count, col_count=column_count)


class PgHandler:
    def traceprov(all_combined_result, version_counter):
        main_result = all_combined_result["result"]
        all_results = dict()
        handler = _get_pg_traceprov_handle("traceprov", version_counter, 1)[0]
        for row_count_str, row_result in main_result.items():
            for column_key, column_result in row_result.items():
                query_num = make_row_column(row_count_str, column_key)
                all_results[query_num] = column_result

        rows = []
        for query_num, query_result in all_results.items():
            rows.extend(handler((query_num, query_result)))
        return rows

    def provsql(all_combined_result, version_counter):
        main_result = all_combined_result["result"]
        all_results = dict()
        handler = _get_provsql_handle("provsql", version_counter, 1)[0]
        for row_count_str, row_result in main_result.items():
            for column_key, column_result in row_result.items():
                row_count_str = str(row_count_str).replace("provsql_", "")
                query_num = make_row_column(row_count_str, column_key)
                mapped_result = column_result
                mapped_result["phase_1"] = mapped_result["provsql"]
                phase_1_result = mapped_result["phase_1"]
                phase_1_result["extras"] = [
                    ({**et, "capture": et["provsql"]})
                    for et in phase_1_result["extras"]
                ]
                all_results[query_num] = column_result
        rows = []
        for query_num, query_result in all_results.items():
            rows.extend(handler((query_num, query_result)))
        return rows

    def muller(all_combined_result, version_counter):
        main_result = all_combined_result["result"]
        all_results = dict()
        handler = _get_muller_handle("muller", version_counter)[0]
        for row_count_str, row_result in main_result.items():
            for column_key, column_result in row_result.items():
                query_num = make_row_column(row_count_str, column_key)
                mapped_result = column_result
                mapped_result["phase_1_2_combined"] = mapped_result["muller"]
                all_results[query_num] = column_result
        rows = []
        for query_num, query_result in all_results.items():
            rows.extend(handler((query_num, query_result)))
        return rows


class DuckDbHandler:

    def _generic_handle(category, all_combined_result, version_counter):
        results = all_combined_result["results"]["result"]["results"]
        all_results = dict()
        for node in results:
            query_num = MockKeyLog(int(node["row_count"]), node["column_count"])
            all_results[query_num] = node
        rows = []
        combined_result = dict()
        combined_result["call_options"] = all_combined_result["call_options"]
        combined_result["results"] = all_results
        handle_duckdb_result(
            category, "all", combined_result, None, rows, version_counter
        )
        return rows

    def traceprov(all_combined_result, version_counter):
        return DuckDbHandler._generic_handle(
            "traceprov", all_combined_result, version_counter
        )

    def traceprov_file(all_combined_result, version_counter):
        raw_results = DuckDbHandler.traceprov(all_combined_result, version_counter)
        for row in raw_results:
            assert hasattr(row, "category")
            if row.category == "base":
                continue
            row.category = "traceprov_file"
        return raw_results

    def smokedduck(all_combined_result, version_counter):
        return DuckDbHandler._generic_handle(
            "smokedduck", all_combined_result, version_counter
        )


def run():
    result_extractor = ResultExtractor()
    parsed = result_extractor.parse_args()

    out_dir = Path(parsed.out_dir)
    dirs, file_dirs = result_extractor.extract_notes()
    if parsed.dry_run:
        return
    assert parsed.dir is not None
    main_dir = Path(parsed.dir)

    version_counter = 0
    all_result_map = dict()
    for current_dir, current_dir_files in zip(dirs, file_dirs, strict=True):
        backend_system, db_system = current_dir
        if backend_system not in all_result_map:
            all_result_map[backend_system] = []
        current_rows: list = all_result_map[backend_system]
        for current_file in current_dir_files:
            version_counter += 1
            if backend_system == "postgres":
                current_result = json_read_file(
                    main_dir / current_file / "main_result.json"
                )
                assert current_result is not None
                if not hasattr(PgHandler, db_system):
                    print("Skipping: ", db_system)
                    continue
                handler = getattr(PgHandler, db_system)
                current_rows.extend(handler(current_result, version_counter))
                continue
            if backend_system == "duckdb":
                current_result = json_read_file(main_dir / current_file / "result.json")
                assert current_result is not None
                if not hasattr(DuckDbHandler, db_system):
                    print("Skipping: ", db_system)
                    continue
                handler = getattr(DuckDbHandler, db_system)
                current_rows.extend(handler(current_result, version_counter))

    for backend_name, backend_result in all_result_map.items():
        set_query_num(backend_result)
        dump_rows_list(
            "log_cost",
            backend_name,
            "all",
            backend_result,
            out_dir,
            MockKeyLog.get_query_id(),
        )


if __name__ == "__main__":
    run()
