import argparse
import json
from typing import NamedTuple
import subprocess


class Options(NamedTuple):
    config: str


def main():
    parser = argparse.ArgumentParser(prog="aggregation-microbench")
    parser.add_argument("-cfg", "--config", required=True, type=str)

    parsed: Options = parser.parse_args()
