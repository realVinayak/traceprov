from pathlib import Path
from typing import NamedTuple

from utils import SystemLabels
from traceprovpy.tools.file_utils import get_nice_num, just_read, just_write, run_query
from traceprovpy.tools.plot_utils import BenchmarkPlot, make_list_query
import duckdb

CATEGORY_MAPPING = [
    ("base", SystemLabels.base),
    ("traceprov", SystemLabels.traceprov),
    ("traceprov_file", SystemLabels.traceprov_file),
    ("gprom", SystemLabels.gprom),
    ("smokedduck", SystemLabels.smokedduck),
    ("muller", SystemLabels.muller),
    ("provsql", SystemLabels.provsql),
]

CATEGORY_MAPPING_INDEX = [res[0] for res in CATEGORY_MAPPING]


class Value(NamedTuple):
    value: float
    value_std: float | None


class Row(NamedTuple):
    system: float
    values: list[Value]


class Table(NamedTuple):
    rows: list[Row]


def table_to_latex(
    table: Table,
    headers: list[str] | None = None,
    precision: int | str = 2,
) -> str:
    n_cols = 1 + len(table.rows[0].values)

    lines = [
        rf"\begin{{tabular}}{{{'c' * n_cols}}}",
        r"\hline",
    ]

    if headers:
        lines.append(" & ".join(headers) + r" \\")
        lines.append(r"\hline")

    rows = sorted(table.rows, key=lambda row: CATEGORY_MAPPING_INDEX.index(row.system))
    for row in rows:
        cells = [row.system]

        for v in row.values:
            if v.value_std is not None and False:
                cells.append(
                    f"{v.value:.{precision}f} " rf"$\pm$ {v.value_std:.{precision}f}"
                )
            else:
                raw_value = v.value
                if precision == "smart":
                    if raw_value > 10:
                        value = get_nice_num(raw_value)
                    elif raw_value > 1:
                        value = round(raw_value, 1)
                    elif raw_value > 0.01:
                        value = round(raw_value, 2)
                    else:
                        value = round(raw_value, 3)
                else:
                    value = round(raw_value, precision)
                cells.append(str(value))

        lines.append(" & ".join(cells) + r" \\")

    lines.extend(
        [
            r"\hline",
            r"\end{tabular}",
        ]
    )

    return "\n".join(lines)


INTERESTING_ROWS = [10, 1_000, 100_000, 10_000_000]
INTERESTING_ROWS_LABELS = ["10", "1K", "100K", "10M"]


def dump_backend_result(backend_name, backend_result, out_dir: Path):
    num_rows = set()
    for res in backend_result:
        num_rows |= {val["query_num_num_rows"] for val in res[-1]}

    num_rows = num_rows.intersection(set(INTERESTING_ROWS))
    num_rows_nice = list(sorted(list(num_rows)))
    time_rows = []
    storage_rows = []
    storage_key = "log_sizes_page_used_size"
    # if backend_name == "postgres":
    #     storage_key = "log_sizes_page_used_size"
    # elif backend_name == "duckdb":
    #     storage_key = "log_sizes_bytes_used_size"
    # else:
    #     print("Backend name", backend_name)
    #     assert False
    print("Using storage key: ", storage_key)
    for result in backend_result:

        if result[0] == "base":
            continue

        def map_data(key, std_key, scale_value=1):
            data_mapped = {
                qres["query_num_num_rows"]: Value(
                    value=scale_value * (qres[key] / qres["query_num_num_rows"]),
                    value_std=(
                        scale_value * (qres[std_key] / qres["query_num_num_rows"])
                        if std_key is not None
                        else None
                    ),
                )
                for qres in result[-1]
            }
            # print(data_mapped)
            return data_mapped

        time_data = map_data("total_usage_time", "total_time_std", 1_000_000)
        storage_data = map_data(storage_key, None)
        category = result[0]
        time_rows.append(
            Row(system=category, values=[time_data[nr] for nr in num_rows_nice])
        )
        storage_rows.append(
            Row(system=category, values=[storage_data[nr] for nr in num_rows_nice])
        )
    time_table = Table(time_rows)
    storage_table = Table(storage_rows)
    headers = ["Category", *INTERESTING_ROWS_LABELS]
    just_write(
        out_dir / f"{backend_name}_time.txt",
        table_to_latex(time_table, headers, precision="smart"),
    )
    just_write(
        out_dir / f"{backend_name}_storage.txt",
        table_to_latex(storage_table, headers, precision="smart"),
    )
    print("### NUM ROWS: ", num_rows)
    print('### "Table"')
    print(time_table)
    print('### "Storage Table"')
    print(storage_table)


def main():
    plot_context = BenchmarkPlot("plot_log_cost", ignore_args=True)
    plot_context.parser.add_argument("--db_dir", required=True)
    parsed = plot_context.parser.parse_args()
    out_dir = plot_context.add_timestamp("db_dir")

    db_dir = Path(parsed.db_dir)
    db_files = list(db_dir.glob("*.db"))
    db_mapping = {file.name.split("_")[-2]: file for file in db_files}
    query = make_list_query(
        just_read("./extractions/log_cost/overview_all_log_cost.sql"), None
    )
    result_mapped = dict()
    for backend_system, db_file in db_mapping.items():
        conn = duckdb.connect(db_file)
        cursor = conn.cursor()
        result_mapped[backend_system] = run_query(cursor, query)
        cursor.close()
        conn.close()
    for backend_system, backend_result in result_mapped.items():
        dump_backend_result(backend_system, backend_result, out_dir)
    # print(result_mapped)


if __name__ == "__main__":
    main()
