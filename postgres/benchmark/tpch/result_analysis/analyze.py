import argparse
import json
import os
from pathlib import Path

from traceprovpy.tools.file_utils import json_read_file, just_write
from traceprovpy.tools.normalized_row import Extendable, Normalizable

import duckdb


class NormalizedPgTPCHRow(Normalizable):
    category: str
    query_num: str
    phase_1: Extendable
    phase_2: Extendable
    log_size: Extendable

    def keys(self):
        return {"category", "query_num", "phase_1", "phase_2", "log_size"}


def handle_traceprov_infer(infer_extras: dict):
    core_results = infer_extras["core_results"]
    total_time = sum(
        [
            core_result["raw_results"][-1]["result"]["explain_time"]
            for core_result in core_results
        ]
    )
    row_counts = sum([core_result["row_count"] for core_result in core_results])
    return dict(time=total_time, row_count=row_counts)


def handle_traceprov_log_size(log_size_extras: dict):
    return json.loads(log_size_extras["captured"][0][0])


def handle_q15_traceprov(query_num: str, query_data: dict) -> dict:
    traceprov_data = query_data["traceprov_15_skippable"]
    phase_1 = [
        dict(explain_time=item["traceprov"][0]["explain_time"])
        for item in traceprov_data["extras"]
    ]
    phase_2 = [
        handle_traceprov_infer(item["traceprov_infer"][0])
        for item in traceprov_data["extras"]
    ]
    log_size = [
        handle_traceprov_log_size(item["traceprov_get_total_layer_size"][0])
        for item in traceprov_data["extras"]
    ]
    args = dict(
        query_num=query_num,
        phase_1=Extendable(phase_1),
        phase_2=Extendable(phase_2),
        log_size=Extendable(log_size),
    )
    return args


def handle_traceprov(query_num: str, query_data: dict) -> dict:
    if query_num == "15":
        return handle_q15_traceprov(query_num, query_data)
    traceprov_data = query_data["traceprov"]
    phase_1 = [
        dict(explain_time=item["explain_time"]) for item in traceprov_data["base"]
    ]
    phase_2 = [
        handle_traceprov_infer(extra["traceprov_infer"][0])
        for extra in traceprov_data["extras"]
    ]
    log_size = [
        handle_traceprov_log_size(extra["traceprov_get_total_layer_size"][0])
        for extra in traceprov_data["extras"]
    ]
    args = dict(
        query_num=query_num,
        phase_1=Extendable(phase_1),
        phase_2=Extendable(phase_2),
        log_size=Extendable(log_size),
    )
    return args


def handle_q15_base(query_num: str, query_data: dict) -> dict:
    base_data = query_data["base_15_skippable"]
    phase_1 = [
        dict(explain_time=item["base"][0]["explain_time"])
        for item in base_data["extras"]
    ]
    return dict(
        query_num=query_num, phase_1=Extendable(phase_1), phase_2=None, log_size=None
    )


def handle_base(query_num: str, query_data: dict, handle_special=True) -> dict:
    if query_num == "15" and handle_special:
        return handle_q15_base(query_num, query_data)
    base_data = query_data["base"]
    contains_timeout = any("timeout" in item for item in base_data["base"])
    if contains_timeout or len(base_data) == 0:
        phase_1 = None
    else:
        phase_1 = [
            dict(explain_time=item["explain_time"]) for item in base_data["base"]
        ]
    return dict(
        query_num=query_num,
        phase_1=Extendable(phase_1) if phase_1 is not None else None,
        phase_2=None,
        log_size=None,
    )


def make_log_size(size: int):
    return dict(log_size_bytes_used_size=size)


def handle_q15_muller(query_num, query_data):
    muller_data = query_data["traceprov_15_skippable"]
    phase_1 = [
        dict(explain_time=item["phase_1_capture"][0]["explain_time"])
        for item in muller_data["extras"]
    ]
    phase_2 = [
        dict(explain_time=item["phase_2_capture"][0]["explain_time"])
        for item in muller_data["extras"]
    ]
    log_size = [
        make_log_size(item["muller_get_log_size"][0]["captured"][0][0])
        for item in muller_data["extras"]
    ]
    args = dict(
        query_num=query_num,
        phase_1=Extendable(phase_1),
        phase_2=Extendable(phase_2),
        log_size=Extendable(log_size),
    )
    return args


def handle_muller(query_num: str, query_data: dict) -> dict:
    if query_num == "15":
        return handle_q15_muller(query_num, query_data)
    muller_data = query_data["phase_1_2_combined"]
    base = muller_data["base"]
    mat = muller_data["materialize"]
    base_contains_timeout = any("timeout" in item for item in base)
    mat_contains_timeout = any("timeout" in item for item in mat)
    if base_contains_timeout or mat_contains_timeout or len(base) == 0 or len(mat) == 0:
        return dict(query_num=query_num, phase_1=None, phase_2=None, log_size=None)
    phase_1 = [dict(explain_time=item["explain_time"]) for item in base]
    phase_2 = [dict(explain_time=item["explain_time"]) for item in mat]
    log_size = [
        make_log_size(item["muller_get_log_size"][0]["captured"][0][0])
        for item in muller_data["extras"]
    ]
    args = dict(
        query_num=query_num,
        phase_1=Extendable(phase_1),
        phase_2=Extendable(phase_2),
        log_size=Extendable(log_size),
    )
    return args


class ResultAnalyzer:

    def __init__(self):
        self.added_base = False

    def gprom(self, key: str, path: Path) -> list[NormalizedPgTPCHRow]:
        print("Handling GProM!", key)
        result = json_read_file(path, True)
        query_results = result["result"]["params_default"]
        rows: list[NormalizedPgTPCHRow] = []
        for query_num, query_data in query_results.items():
            for category, category_data in query_data.items():
                base_row_args = handle_base(
                    query_num, dict(base=category_data), handle_special=False
                )
                base_args = {**base_row_args, "category": category}
                rows.append(NormalizedPgTPCHRow(**base_args))
        return rows

    def muller(self, key: str, path: Path) -> list[NormalizedPgTPCHRow]:
        print("Handling Muller!", key)
        result = json_read_file(path, True)
        query_results = result["result"]["muller_params_default"]
        rows: list[NormalizedPgTPCHRow] = []
        for query_num, query_data in query_results.items():
            row_args = handle_muller(query_num, query_data)
            args = {**row_args, "category": key}
            rows.append(NormalizedPgTPCHRow(**args))
        return rows

    def traceprov(
        self,
        key: str,
        path: Path,
    ) -> list[NormalizedPgTPCHRow]:
        print("Handling TraceProv!", key)
        result = json_read_file(path, True)
        query_results = result["result"]["params_default"]
        base_rows: list[NormalizedPgTPCHRow] = []
        traceprov_rows: list[NormalizedPgTPCHRow] = []
        for query_num, query_data in query_results.items():
            tp_row_args = handle_traceprov(query_num, query_data)
            tp_args = {**tp_row_args, "category": key}
            traceprov_rows.append(NormalizedPgTPCHRow(**tp_args))
            base_row_args = handle_base(query_num, query_data)
            base_args = {**base_row_args, "category": "base"}
            base_rows.append(NormalizedPgTPCHRow(**base_args))
        all_rows = [*traceprov_rows]
        if not self.added_base:
            all_rows = [*base_rows, *all_rows]
        self.added_base = True
        return all_rows


def main():
    parser = argparse.ArgumentParser("tpch_analyzer")
    parser.add_argument("--dir", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--out_dir", required=True)
    parsed = parser.parse_args()
    config = json_read_file(parsed.config, True)
    out_dir = Path(parsed.out_dir)
    os.makedirs(out_dir, exist_ok=True)
    assert config is not None
    in_dir = Path(parsed.dir)
    normalized_rows: list[NormalizedPgTPCHRow] = []
    analyze = ResultAnalyzer()
    for key, item in config.items():
        result_file_path: Path = in_dir / item["file"] / "main_result.json"
        assert result_file_path.exists(), f"Expected {result_file_path} to exist!"
        _rows = getattr(analyze, item["handler"])(key, result_file_path)
        normalized_rows.extend(_rows)

    flatted = [row for group in normalized_rows for row in group.normalize()]
    path = just_write(out_dir / "flat.json", json.dumps(flatted))
    if isinstance(path, Path):
        path = path.as_posix()
    conn = duckdb.connect(out_dir / "analyze.db")
    cursor = conn.cursor()
    cursor.execute(
        f"create or replace table normalized as (select * from read_json_auto('{path}'))"
    )
    cursor.close()
    conn.close()


if __name__ == "__main__":
    main()
