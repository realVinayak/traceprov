import argparse
import os
from pathlib import Path

from matplotlib import patches, pyplot as plt

from utils import ERROR_HATCHES, SYSTEM_COLORS, SYSTEM_MARKERS, PlotErrors, generic_dump_legends
from plot_overview import CATEGORY_LABEL, CATEGORY_MAPPING

import matplotlib.lines as mlines

def get_true_label(sysname):
    if sysname == 'gprom' and 'gprom_window_heuristics' in CATEGORY_LABEL:
        sysname = 'gprom_window_heuristics'
    return CATEGORY_LABEL[sysname]

def plot_filled_legends(parsed, out_dir):
    handles = []
    for system in parsed.system:
        all_kwargs = dict()
        true_system_name = get_true_label(system)
        if parsed.color:
            all_kwargs['color'] = SYSTEM_COLORS[true_system_name]
        all_kwargs['label'] = true_system_name
        patch = patches.Patch(**all_kwargs)
        handles.append(patch)
    for error_key in parsed.error:
        error_value = getattr(PlotErrors, error_key)
        error_hatch = ERROR_HATCHES[error_value]
        patch = patches.Patch(
            facecolor="lightgray",
            label=error_value,
            hatch=error_hatch,
            edgecolor="black",
        )
        handles.append(patch)
    fig, ax = plt.subplots()
    generic_dump_legends(fig, ax, out_dir, handles, len(handles))


def plot_marker_legends(parsed, out_dir):
    fig, ax = plt.subplots()
    handles_row_1 = []
    for system in parsed.system:
        system_label = get_true_label(system)
        all_kwargs = dict(
            label=system_label,
            markerfacecolor="none",
            markeredgecolor=SYSTEM_COLORS[system_label],
            linestyle="none",
        )
        if parsed.color:
            all_kwargs['color'] = SYSTEM_COLORS[system_label]
        if parsed.marker:
            all_kwargs['marker'] = SYSTEM_MARKERS[system_label]
        patch = mlines.Line2D(
            [],
            [],
            **all_kwargs
        )
        handles_row_1.append(patch)
    generic_dump_legends(fig, ax, out_dir, handles_row_1, len(handles_row_1))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--system", nargs='+', required=True)
    parser.add_argument("--color", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument('--marker',action=argparse.BooleanOptionalAction, default=False)
    parser.add_argument("--error", nargs='*', default=[])
    parser.add_argument("--error_new_line", action=argparse.BooleanOptionalAction, default=False)
    parser.add_argument("--out_dir", required=True)


    parsed = parser.parse_args()
    systems = parsed.system
    for system in systems:
        assert system in CATEGORY_LABEL or system == 'gprom'

    handler = plot_marker_legends if parsed.marker else plot_filled_legends
    out_dir = Path(parsed.out_dir)
    os.makedirs(out_dir, exist_ok=True)
    handler(parsed, out_dir)


if __name__ == '__main__':
    main()
