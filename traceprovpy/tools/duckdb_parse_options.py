# some basic options that get reused for duckdb.

import argparse


def make_duckdb_parse():
    parser = argparse.ArgumentParser()
    parser.add_argument("--db", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--suff", required=True)
    parser.add_argument("--sd_mode", choices=["old", "new"], default=None)
    parser.add_argument("--sd_extension_path", required=False)
    parser.add_argument("--sample_inference", choices=["all", "sample"], default=False)
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
    return parser
