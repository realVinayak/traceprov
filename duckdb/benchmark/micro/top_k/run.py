import argparse
import os
import random

sibling_path = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "..", "selectivity")
)

import run as selectivity_run

# to make sampling reproducible.
random.seed(10)


def run():
    selectivity_run.run()


if __name__ == "__main__":
    run()
