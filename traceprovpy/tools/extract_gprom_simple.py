# A more simpler interface to interact with gprom.
# Simply takes in the connection params object, and handles things sanely.

import argparse
import json
from pathlib import Path
from typing import Literal, NamedTuple
from traceprovpy.tools.file_utils import just_read, just_write
from traceprovpy.tools.run_with_timeout import (
    ConnectionParams,
    DuckDBConnectionParams,
    MakeKeySelection,
)
import os

JOIN = "join"
WINDOW = "window"
HEURISTICS = "heuristics"

HEURISTICS_OPTIONS = ["-heuristic_opt TRUE"]
LATERAL_OPTIONS = ["-lateral_rewrite TRUE"]
UNNEST_OPTIONS = ["-unnest_rewrite TRUE"]


class GpromOptions(NamedTuple):
    mode: Literal["join", "window", "join_composable"]
    heuristics: bool = False
    is_lateral: bool = False
    is_unnest: bool = False

    def from_parsed(self, parsed):
        return self._replace(is_lateral=parsed.lateral)

    @staticmethod
    def add_parse_options(parser):
        parser.add_argument(
            "--lateral", action=argparse.BooleanOptionalAction, default=False
        )
        parser.add_argument(
            "--is_unnest", action=argparse.BooleanOptionalAction, default=False
        )
        parser.add_argument(
            "--backend", choices=["postgres", "duckdb"], default="postgres"
        )

    def to_str(self):
        return f"({self.mode} - {self.heuristics})"

    @staticmethod
    def from_str(self_str: str):
        mode = "window"
        if "join" in self_str:
            if "composable" in self_str:
                mode = "join_composable"
            else:
                mode = "join"
        heu = "True" in self_str or "heu" in self_str
        return GpromOptions(mode=mode, heuristics=heu)

    def safe_key(self, ignore_prefix=False):
        if not ignore_prefix:
            parts = ["gprom", self.mode]
        else:
            parts = [self.mode]
        if self.heuristics:
            parts.append("heuristics")
        return "_".join(parts)


GPROM_OPTIONS_MAPPING = dict(
    join=GpromOptions(mode="join"),
    window=GpromOptions(mode="window"),
    join_heuristics=GpromOptions(mode="join", heuristics=True),
    window_heuristics=GpromOptions(mode="window", heuristics=True),
    join_composable=GpromOptions(mode="join_composable", heuristics=False),
    join_composable_heuristics=GpromOptions(mode="join_composable", heuristics=True),
)

gprom_modes = {
    "join": [],
    "window": ["-prov_instrument_agg_window", "-prov_use_composable"],
    "join_composable": ["-prov_instrument_agg_window FALSE", "-prov_use_composable"],
}


def gprom_from_parsed(parsed, input_file):

    connection_params = ConnectionParams.make_from_parsed(parsed)
    gprom_options = GpromOptions.from_parsed(parsed)
    return gprom_from_file(gprom_options, connection_params, input_file)


def gprom_from_file(
    options: GpromOptions,
    connection_param: ConnectionParams | DuckDBConnectionParams,
    file_name: str,
):
    out_file = "/tmp/gprom_extracted_temp.sql"
    os.system(f"rm -f {out_file}")
    generic_options = dict(Pexecutor="wf", queryFile=file_name, Pout_file=out_file)
    backend = None
    if isinstance(connection_param, ConnectionParams):
        backend = "postgres"
        base_user_options = dict(
            user=connection_param.user,
            passwd=connection_param.password,
            host=connection_param.host,
            port=connection_param.port,
            db=connection_param.database,
            # operator_verbose="TRUE"
        )
    elif isinstance(connection_param, DuckDBConnectionParams):
        backend = "duckdb"
        base_user_options = dict(db=connection_param.db)
    else:
        assert 0, "Got invalid instance!"

    base_user_options = {**base_user_options, **generic_options}

    flattened = " ".join(f"-{key} {value}" for key, value in base_user_options.items())
    execute_cmd_raw = f"LD_LIBRARY_PATH=/usr/bin/libduck_prebuilt/ PATH=$PATH:/home/realv/projects/gprom/bld/bin/ gprom -backend {backend} {flattened}"
    extra_options = gprom_modes[options.mode]
    if options.heuristics:
        extra_options = [*extra_options, *HEURISTICS_OPTIONS]
    if options.is_lateral:
        extra_options = [*extra_options, *LATERAL_OPTIONS]
    if options.is_unnest:
        extra_options = [*extra_options, *UNNEST_OPTIONS]

    gprom_cmd_list = [execute_cmd_raw, *extra_options]
    gprom_cmd = " ".join(gprom_cmd_list)
    print(gprom_cmd)
    os.system(gprom_cmd)
    try:
        with open(out_file) as f:
            gprom_sql = f.read()
    except FileNotFoundError:
        raise Exception("Error creating gprom file!")

    return gprom_sql


def gprom_handle_rewrite_single_row_mode(
    query_name: str,
    base_query_path: Path,
    temp_dir: Path,
):
    query = just_read(base_query_path)
    assert query is not None
    query_out_path = temp_dir / "query_out.json"
    query_rewritten = temp_dir / f"query_rewritten_{query_name}.sql"
    query = query.replace(";", "")
    query = f"copy (select * from ({query})) to '{query_out_path.as_posix()}'"
    just_write(query_rewritten, query)
    return query_rewritten, query_out_path


def gprom_add_predicates(
    query_name: str,
    query_path: Path | str,
    result_path: Path,
    temp_dir: Path,
    query_keys: dict,
):
    raw_query_result = just_read(result_path).splitlines()
    assert isinstance(raw_query_result, list)
    query_result = list(map(json.loads, raw_query_result))
    print(query_result)
    if not isinstance(query_path, str):
        query = just_read(query_path)
    else:
        query = query_path
    files: list[Path] = []
    query_keys = [key.lower() for key in query_keys]
    for row_result_idx, row_result in enumerate(query_result):
        filter_pack = {
            key: value
            for (key, value) in row_result.items()
            if not key.lower().startswith("prov_") and key.lower() in query_keys
        }
        preprocessor = MakeKeySelection(filter_pack)
        filtered = preprocessor.preprocess(query)
        out_dir = temp_dir / str(row_result_idx)
        os.makedirs(out_dir, exist_ok=True)
        query_rewritten_file = out_dir / f"filtered_{query_name}.sql"
        just_write(query_rewritten_file, filtered)
        files.append(query_rewritten_file)
    return files
