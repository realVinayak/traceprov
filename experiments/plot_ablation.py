from matplotlib import pyplot as plt
import numpy as np

from plot_log_cost import Row, Table, Value, table_to_latex, smart_float_print
from utils import make_category_data_mapped
from traceprovpy.tools.file_utils import just_read, just_write, run_query
from traceprovpy.tools.plot_utils import BenchmarkPlot
import duckdb


class AblationLabels:
    traceprov_simple = "Vanilla"
    traceprov = "J.R. + Partition"
    traceprov_partition = "Partition"
    traceprov_join_rewrite = "J.R."
    traceprov_join_rewrite_no_pushdown = "J.R. (w/o Zone Maps)"

class AblationSimpleLabels:
    traceprov_simple = "$\\nil$"
    traceprov = "J.R. + P + Z"
    traceprov_partition = "P"
    traceprov_join_rewrite = "J.R + Z"
    traceprov_join_rewrite_no_pushdown = "J.R."


CATEGORIES = [
    ("traceprov_simple", AblationLabels.traceprov_simple),
    ("traceprov_partition", AblationLabels.traceprov_partition),
    (
        "traceprov_join_rewrite_no_pushdown",
        AblationLabels.traceprov_join_rewrite_no_pushdown,
    ),
    ("traceprov_join_rewrite", AblationLabels.traceprov_join_rewrite),
    ("traceprov", AblationLabels.traceprov),
]


CATEGORIES_ORDERED = list([cat[0] for cat in CATEGORIES])


def get_quantiles(values):
    values = list(values)
    print("VALUES: ", values)
    min_value = min(values)
    max_value = max(values)
    quarties = list(np.quantile(values, [0.25, 0.5, 0.75]))
    return [min_value, *quarties, max_value]


def run():
    plot_context = BenchmarkPlot("plot_ablation", ignore_args=True)
    plot_context.parser.add_argument("--db", nargs="+")
    parsed = plot_context.parser.parse_args()
    setattr(parsed, "first_db", parsed.db[0])
    # out_dir = plot_context.add_timestamp("first_db")
    query = just_read("./extractions/ablation/offset_query_ablation.sql")
    all_results = []
    for db in parsed.db:
        conn = duckdb.connect(db)
        cursor = conn.cursor()
        results = run_query(cursor, query)
        all_results.extend(results)
        cursor.close()
        conn.close()
    print(all_results)
    results_mapping = {res[0]: res[1] for res in all_results}
    simple_traceprov = make_category_data_mapped(results_mapping["traceprov_simple"])
    slowdown_results = dict()
    quantile_results = dict()
    slowdown_table_results = dict()
    query_list = list(map(str, range(1, 23)))
    for result in all_results:
        traceprov_type, traceprov_results = result
        # if traceprov_type == "traceprov_simple":
        #     continue
        current_mapped = make_category_data_mapped(traceprov_results)
        all_query_list = list(current_mapped.keys())
        slowdown_results[traceprov_type] = [
            current_mapped[query]["phase_2_profile_latency"] for query in current_mapped
        ]
        quantile_results[traceprov_type] = get_quantiles(
            current_mapped[query]["phase_2_profile_latency"] for query in all_query_list
        )
        slowdown_table_results[traceprov_type] = Row(getattr(AblationSimpleLabels, traceprov_type), [
            Value(smart_float_print(simple_traceprov[query]['phase_2_profile_latency'] / current_mapped[query]['phase_2_profile_latency']), None)
            for query in query_list
        ])
    backtrace_fig, backtrace_axis = plt.subplots(1, 1, figsize=(4, 2))
    categories_to_show = CATEGORIES_ORDERED[0:]
    slowdown_data = [slowdown_results[cat] for cat in categories_to_show]
    bplot = backtrace_axis.boxplot(
        slowdown_data,
        labels=[res[1] for res in CATEGORIES[0:]],
        whis=(0, 100),
        showfliers=False,
    )
    backtrace_axis.set_yscale("log")
    out_dir = plot_context.add_timestamp("first_db")
    print(quantile_results)
    rows = []
    slowdown_rows = []
    for cat in CATEGORIES_ORDERED:
        values = list(
            [
                Value(round(1_000 * float(val), 2), value_std=None)
                for val in quantile_results[cat]
            ]
        )
        rows.append(Row(system=getattr(AblationLabels, cat), values=values))
        slowdown_rows.append(slowdown_table_results[cat])
    table = Table(rows)
    headers = ["Mode", "Min", "25\%", "50\%", "75\%", "Max"]
    latex_table = table_to_latex(
        table, headers=headers, precision="raw", assume_sorted=True
    )
    backtrace_fig.savefig(out_dir / "ablation_slowdown.pdf", bbox_inches="tight")
    just_write(out_dir / "raw_values.tex", latex_table)

    latex_table_all = Table(slowdown_rows)
    headers = ["Mode", *query_list]
    slowdown_latex_table = table_to_latex(
        latex_table_all, headers=headers, precision="raw", assume_sorted=True
    )
    just_write(out_dir / "slowdown_values.tex", slowdown_latex_table)

if __name__ == "__main__":
    run()
