# some basic options that get reused for duckdb.

import argparse


def make_duckdb_parse():
    parser = argparse.ArgumentParser()
    parser.add_argument("--db", required=True)
    parser.add_argument("--exe", required=True)
    return parser
