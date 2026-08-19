# some basic options that get reused for duckdb.

import argparse
import math

from traceprovpy.tools.duckdb_inference import DuckDBDriverOptions
from traceprovpy.tools.file_utils import get_tmp_file, set_tmp_file


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
    # For DuckDB, we strictly use rowids for everything.
    # So, using the optimized setting here by default is reasonable.
    parser.add_argument(
        "--optimized", action=argparse.BooleanOptionalAction, default=True
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
    parser.add_argument("--sample_num", type=str, default="100")
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
        "--traceprov_sample_fast", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--traceprov_join_rewrite", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--traceprov_use_column_log",
        action=argparse.BooleanOptionalAction,
        default=False,
    )
    parser.add_argument(
        "--mat_capture", action=argparse.BooleanOptionalAction, default=False
    )
    parser.add_argument(
        "--capture_sd_stats", action=argparse.BooleanOptionalAction, default=False
    )
    # parser.add_argument("--sample_inference_limit", type=str, default=10)
    DuckDBDriverOptions.add_parse_options(parser)
    parser.add_argument("--g_tmp_dir", required=False, default=get_tmp_file())
    return parser


SMART_TOKEN = "SMART"


# Try to be smart and automatically try to compose a suff string out of the options.
def traceprov_handle_suffix(parsed):
    print("Parsed: ", parsed)
    set_tmp_file(parsed.g_tmp_dir)
    if parsed.traceprov_fast or parsed.traceprov_sample_fast:
        parsed.traceprov_use_merge_chunks = True
        parsed.traceprov_use_compact = True
        parsed.traceprov_use_table_stats = True
        parsed.optimized = True
    if parsed.sample_inference is not None and parsed.traceprov_sample_fast:
        parsed.traceprov_use_partition_in_agg = True
        parsed.traceprov_use_join_filter_rewrite = True
        parsed.traceprov_use_filter_pushdown = True
    if parsed.traceprov_join_rewrite:
        parsed.traceprov_use_join_filter_rewrite = True
        parsed.traceprov_use_filter_pushdown = True
    if parsed.threads > 1 and parsed.sample_inference is not None:
        parsed.traceprov_use_column_log = True
    if SMART_TOKEN not in parsed.suff:
        return
    suff = DuckDBDriverOptions.get_suffix(parsed)
    parsed.suff = parsed.suff.replace(SMART_TOKEN, suff)


def is_integer(val: str):
    try:
        parsed = int(val)
        return True
    except ValueError:
        return False


def _traceprov_get_sample_count(parsed, result_count):
    sample_inference_limit: str = parsed.sample_num
    if parsed.sample_inference == "all":
        return result_count
    if is_integer(sample_inference_limit):
        return int(sample_inference_limit)

    assert ":" in sample_inference_limit
    split_mode = sample_inference_limit.split(":")
    assert len(split_mode) == 2
    mode, value = tuple(split_mode)
    value = int(value)
    if mode == "percent":
        sample_value = math.ceil((result_count * value) / 100)
        assert sample_value > 0
    else:
        raise Exception(f"Got {mode} mode!")
    return sample_value


def traceprov_get_sample_count(parsed, result_count):
    sample_value = _traceprov_get_sample_count(parsed, result_count)
    clamped = min(sample_value, result_count)
    print("Using clamped value: ", clamped, result_count)
    return clamped
