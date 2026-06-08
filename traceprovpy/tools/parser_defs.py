import argparse

from traceprovpy.tools.duckdb_parse_options import make_duckdb_parse


def make_duckdb_selectivity_parser():
    base_parser = make_duckdb_parse()
    base_parser.add_argument("--num_groups", required=True, type=int)
    base_parser.add_argument("--config", required=True)
    base_parser.add_argument(
        "--random", action=argparse.BooleanOptionalAction, default=True
    )
    base_parser.add_argument(
        "--top_k_mode", action=argparse.BooleanOptionalAction, default=False
    )
    base_parser.add_argument("--mode", choices=["pre", "post"], default="post")
    return base_parser
