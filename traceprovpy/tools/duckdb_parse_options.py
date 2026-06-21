# some basic options that get reused for duckdb.

import argparse

from traceprovpy.tools.duckdb_inference import DuckDBDriverOptions


def make_duckdb_parse():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", required=True)
    parser.add_argument("--suff", required=True)
    parser.add_argument("--sd_mode", choices=["old"], default=None)
    parser.add_argument("--sd_extension_path", required=False)
    parser.add_argument(
        "--sample_inference", choices=["all", "sample", "limit"], default=None
    )
    parser.add_argument(
        "--validate", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--mat_infer", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--optimized", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--agg_optimized", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--strict", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--crash_on_error", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument("--infer", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--sample_num", type=int, default=100)
    parser.add_argument("--graph_dir", type=str, required=False)
    # Backtrace all for SD?
    parser.add_argument(
        "--backtrace_all_sd", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--single_row_mode",
        default=False,
        action=argparse.BooleanOptionalAction,
    )
    parser.add_argument(
        "--traceprov_fast", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--traceprov_use_column_log",
        action=argparse.BooleanOptionalAction,
        default=False,
    )
    parser.add_argument(
        "--mat_capture", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument("--sample_inference_limit", type=int, default=10)
    DuckDBDriverOptions.add_parse_options(parser)
    return parser


SMART_TOKEN = "SMART"


# Try to be smart and automatically try to compose a suff string out of the options.
def traceprov_handle_suffix(parsed):
    if parsed.traceprov_fast:
        parsed.traceprov_use_merge_chunks = True
        parsed.traceprov_use_compact = True
        parsed.traceprov_use_table_stats = True
        parsed.optimized = True
    if (
        parsed.threads > 1
        and parsed.sample_inference is not None
        and parsed.traceprov_use_partition_in_agg
    ):
        parsed.traceprov_use_column_log = True
    if SMART_TOKEN not in parsed.suff:
        return
    suff = DuckDBDriverOptions.get_suffix(parsed)
    parsed.suff = parsed.suff.replace(SMART_TOKEN, suff)
