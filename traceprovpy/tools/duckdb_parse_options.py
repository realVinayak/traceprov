# some basic options that get reused for duckdb.

import argparse
import re


def make_duckdb_parse():
    parser = argparse.ArgumentParser()
    parser.add_argument("--db", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--suff", required=True)
    parser.add_argument("--sd_mode", choices=["old", "new"], default=None)
    parser.add_argument("--sd_extension_path", required=False)
    parser.add_argument(
        "--traceprov_use_partition_in_agg",
        action=argparse.BooleanOptionalAction,
        default=False,
    )
    parser.add_argument(
        "--traceprov_use_partition_in_log",
        action=argparse.BooleanOptionalAction,
        default=False,
    )
    parser.add_argument(
        "--traceprov_use_row_in_agg_partition",
        action=argparse.BooleanOptionalAction,
        default=False,
    )
    parser.add_argument("--sample_inference", choices=["all", "sample"], default=None)
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
    parser.add_argument("--infer", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--sample_num", type=int, default=100)
    parser.add_argument("--graph_dir", type=str, required=False)
    parser.add_argument("--threads", type=int, default=1)

    parser.add_argument(
        "--traceprov_use_implicit_union",
        action=argparse.BooleanOptionalAction,
        default=False,
    )
    parser.add_argument(
        "--traceprov_dry_run_derivation",
        action=argparse.BooleanOptionalAction,
        default=False,
    )
    parser.add_argument(
        "--traceprov_use_merge_chunks",
        action=argparse.BooleanOptionalAction,
        default=False,
    )
    parser.add_argument(
        "--traceprov_combine_in_memory",
        action=argparse.BooleanOptionalAction,
        default=False,
    )
    parser.add_argument(
        "--traceprov_split_combine",
        action=argparse.BooleanOptionalAction,
        default=False,
    )
    return parser
