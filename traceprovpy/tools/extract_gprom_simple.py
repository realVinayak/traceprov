# A more simpler interface to interact with gprom.
# Simply takes in the connection params object, and handles things sanely.

import argparse
from typing import Literal, NamedTuple
from traceprovpy.tools.run_with_timeout import ConnectionParams
import os

JOIN = "join"
WINDOW = "window"
HEURISTICS = "heuristics"

HEURISTICS_OPTIONS = ["-heuristic_opt TRUE"]


class GpromOptions(NamedTuple):
    mode: Literal["join", "window"]
    heuristics: bool = False

    @staticmethod
    def from_parsed(parsed):
        assert parsed.gp_mode == "join" or parsed.gp_mode == "window"
        return GpromOptions(mode=parsed.gp_mode, heuristics=parsed.heu)

    @staticmethod
    def add_parse_options(parser):
        parser.add_argument("--gp_mode", required=True, type=str)
        parser.add_argument(
            "--heu", action=argparse.BooleanOptionalAction, default=False
        )


GPROM_OPTIONS_MAPPING = dict(
    join=GpromOptions(mode="join"),
    window=GpromOptions(mode="window"),
    join_heuristics=GpromOptions(mode="join", heuristics=True),
    window_heuristics=GpromOptions(mode="window", heuristics=True),
)

gprom_modes = {
    "join": [],
    "window": ["-prov_instrument_agg_window", "-prov_use_composable"],
}


def gprom_from_parsed(parsed, input_file):

    connection_params = ConnectionParams.make_from_parsed(parsed)
    gprom_options = GpromOptions.from_parsed(parsed)
    return gprom_from_file(gprom_options, connection_params, input_file)


def gprom_from_file(
    options: GpromOptions, connection_param: ConnectionParams, file_name: str
):
    out_file = "/tmp/gprom_extracted_temp.sql"
    os.system(f"rm -f {out_file}")
    base_user_options = dict(
        user=connection_param.user,
        passwd=connection_param.password,
        host=connection_param.host,
        port=connection_param.port,
        db=connection_param.database,
        Pexecutor="sql",
        Loperator_verbose="TRUE",
        queryFile=file_name,
        Poutfile=out_file,
    )

    flattened = " ".join(f"-{key} {value}" for key, value in base_user_options.items())
    execute_cmd_raw = f"gprom -backend postgres {flattened}"
    extra_options = gprom_modes[options.mode]
    if options.heuristics:
        extra_options = [*extra_options, *HEURISTICS_OPTIONS]

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
