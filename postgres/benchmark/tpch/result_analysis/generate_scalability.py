import argparse
from pathlib import Path
from typing import NamedTuple
import duckdb
from traceprovpy.tools.file_utils import just_read
import matplotlib.lines as mlines
from matplotlib import pyplot as plt


def resolve_value(raw_value):
    if raw_value < 0:
        return "-"
    # if raw_value > 100 or raw_value < 0.01:
    #     # give up.
    #     return f"{raw_value:.2e}"
    if raw_value >= 1_000:
        return f"{round(raw_value / 1_000, 1)} K"
    if raw_value > 1:
        return str(round(raw_value, 1))
    if raw_value <= 1 and raw_value > 0:
        if raw_value >= 0.1:
            return str(round(raw_value, 2))
        else:
            return str(round(raw_value, 3))
        # return f"{raw_value:.2e}"
    return str(round(raw_value, 2))


def get_row(raw_result):
    sf = raw_result[0]
    category = raw_result[1]
    if sf == 1 and category == "base":
        time_key = "normalization_value"
    else:
        time_key = "normalized_time"
    normal_results = raw_result[2]
    mapped = {
        cell["query_num"]: resolve_value(cell[time_key]) for cell in normal_results
    }
    # print(mapped)
    queries = list(range(1, 23))
    query_values = [mapped.get(str(q), resolve_value(-1)) for q in queries]
    return [sf, category, *list(query_values)]


CATEGORY_LABELS = [
    ("base", "Base"),
    ("gprom_gprom_window_heuristics", "GProM"),
    ("muller", "SQLProv"),
    ("traceprov_stats", "TraceProv"),
]


def category_label(cat):
    cat_index = category_index(cat)
    assert cat_index != -1
    return CATEGORY_LABELS[cat_index][1]


def category_index(cat):
    for idx, (target, _) in enumerate(CATEGORY_LABELS):
        if cat == target:
            return idx
    return -1


def filter_rows(row):
    return category_index(row[1]) != -1


# def add_thick_line(ax, table, row, column_count):
#     y_pos = table[row, 0].get_y()
#     x_start = table[row, 0].get_x()
#     x_end = (
#         table[row, column_count - 1].get_x() + table[row, column_count - 1].get_width()
#     )
#     line = mlines.Line2D(
#         [x_start, x_end],
#         [y_pos, y_pos],
#         transform=ax.transData,
#         color="black",
#         linewidth=2,
#     )
#     ax.add_line(line)


def add_thick_line(fig, ax, table, row, column_count):
    renderer = fig.canvas.get_renderer()
    inv = ax.transAxes.inverted()

    # get x from first and last cell
    first_cell = table[row, 0]
    last_cell = table[row, column_count - 1]

    bbox_first = first_cell.get_window_extent(renderer)
    bbox_last = last_cell.get_window_extent(renderer)

    x_start = inv.transform((bbox_first.x0, 0))[0]
    x_end = inv.transform((bbox_last.x1, 0))[0]
    y = inv.transform((0, bbox_first.y0))[1]

    line = mlines.Line2D(
        [x_start, x_end],
        [y, y],
        transform=ax.transAxes,
        color="black",
        linewidth=2,
        clip_on=False,
    )
    ax.add_line(line)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--result_dir", required=True, type=str)
    parsed = parser.parse_args()
    result_dir = Path(parsed.result_dir)

    sf_1_path = result_dir / "1" / "analyze.db"
    sf_10_path = result_dir / "10" / "analyze.db"
    con = duckdb.connect()
    con.execute(f"attach '{sf_1_path.as_posix()}' as scale_1")
    con.execute(f"attach '{sf_10_path.as_posix()}' as scale_10")

    cursor = con.cursor()
    scale_query = just_read("scale_query.sql")
    cursor.execute(scale_query)
    results = cursor.fetchall()
    rows = list(map(get_row, results))
    rows = [row for row in rows if filter_rows(row)]
    rows = sorted(rows, key=lambda row: (row[0], category_index(row[1])))
    rows = [[*map(str, row[:1]), category_label(row[1]), *row[2:]] for row in rows]
    print(rows)
    fig, ax = plt.subplots(figsize=(8, 2))
    ax.axis("off")  # Hide the plot axes completely
    labels = ["SF", "Category", *map(lambda q: f"q{q}", range(1, 23))]
    table = ax.table(
        cellText=rows,
        colLabels=labels,
        loc="center",
        cellLoc="center",
    )
    table.auto_set_font_size(False)
    table.set_fontsize(11)

    # column widths — auto fit to content
    table.auto_set_column_width(col=list(range(1, len(labels))))
    for (row, col), cell in table.get_celld().items():
        cell.set_linewidth(0.85)
    table.scale(1, 1.4)

    for row in range(len(rows) + 1):
        table[row, 0].set_width(0.12)
    for (_, col), cell in table.get_celld().items():
        # print(cell.PAD).
        ...
        # print(col)
        # if col != 1:
        #     cell.PAD = 0.08  # default is around 0.1
        # else:
        #     cell.PAD = 0.04
    # table.set_fontsize()

    last_sf_1 = max([idx for idx, row in enumerate(rows) if row[0] == "1"]) + 1
    fig.canvas.draw()
    add_thick_line(fig, ax, table, 0, len(labels))
    add_thick_line(fig, ax, table, last_sf_1, len(labels))
    fig.savefig(result_dir / "scalability.pdf", bbox_inches="tight")


if __name__ == "__main__":
    main()
