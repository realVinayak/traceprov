# A more simpler interface to interact with gprom.
# Simply takes in the connection params object, and handles things sanely.

import argparse
from typing import Literal, NamedTuple
from traceprovpy.tools.run_with_timeout import ConnectionParams, DuckDBConnectionParams
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
        heu = "True" in self_str
        return GpromOptions(mode=mode, heuristics=heu)

    def safe_key(self):
        parts = ["gprom", self.mode]
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
