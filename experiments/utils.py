from functools import reduce
from pathlib import Path
from typing import NamedTuple

from traceprovpy.tools.plot_utils import BenchmarkPlot


class DataOptions(NamedTuple):
    sf: str
    db_name: str
    mode: str

    def to_db(self):
        return f"data_{self.sf}_{self.db_name}_{self.mode}.db"

    def get_db_name(self):
        if self.db_name in ["duckdb", "newduckdb"]:
            return "duckdb"
        if self.db_name in ["postgres"]:
            return "postgres"
        assert False, f"Got db name: {self.db_name}"

    def true_sf(self):
        sf_split = self.sf.split("_")
        return int(sf_split[-1])

    def normalize_sf(self):
        return self._replace(sf="")

    def get_timeout_value(self):
        resolved_db_name = self.get_db_name()
        if resolved_db_name == "duckdb":
            return 300
        if resolved_db_name == "postgres":
            return 600
        assert False, f"Expected to not reach here!"


def get_options_split(parsed):
    db = Path(parsed.db).name
    print(db)
    db = db.replace(".db", "")
    assert db.count("_") == 4
    splitted = db.split("_")
    assert splitted[0] == "data"
    sf = f"{splitted[1]}_{splitted[2]}"
    return DataOptions(sf=sf, db_name=splitted[3], mode=splitted[4])


class SystemLabels:
    base = "Base"
    traceprov = "TraceProv"
    gprom = "GProM"
    smokedduck = "SmokedDuck"
    muller = "Müller"
    provsql = "ProvSQL"


# colors = [
#     "tab:blue",
#     "tab:orange",
#     "tab:green",
#     "tab:red",
#     "tab:purple",
#     "tab:brown",
#     "tab:pink",
#     "tab:gray",
#     "tab:olive",
#     "tab:cyan",
#     "m",
#     "k",
# ]
SYSTEM_COLORS = {
    SystemLabels.base: "#3E3F3E",
    SystemLabels.traceprov: "#0072B2",
    SystemLabels.gprom: "#CC79A7",
    SystemLabels.smokedduck: "#D55E00",
    SystemLabels.muller: "#009E73",
    SystemLabels.provsql: "#33006A",
}

SYSTEM_MARKERS = {
    SystemLabels.base: "o",
    SystemLabels.traceprov: "P",
    SystemLabels.gprom: "^",
    SystemLabels.smokedduck: "s",
    SystemLabels.muller: "d",
    SystemLabels.provsql: "s",
}


class PlotErrors:
    not_available = "N/A"
    timeout = "Timeout"
    i_o_full = "I/O Full"
    oom = "OOM"


ERROR_PATCH_MAPPING = [
    (PlotErrors.not_available, "oo"),
    (PlotErrors.timeout, "///"),
    (PlotErrors.i_o_full, "|||"),
    (PlotErrors.oom, "xx"),
]

ERROR_HATCHES = {er[0]: er[1] for er in ERROR_PATCH_MAPPING}


def make_category_data_mapped(category_data):
    return {node["query_num"]: node for node in category_data}


def get_query_error_mapping(query_errors):
    def _reduce(mapping, current):
        query, query_error = current
        return {**mapping, query_error: [*(mapping.get(query_error, [])), query]}

    return reduce(_reduce, query_errors, dict())


def get_main_error(fail_reason: str, data_options: DataOptions):
    if fail_reason == "" or fail_reason is None:
        return None
    fail_reason = fail_reason.lower()
    if fail_reason == "timeout":
        return PlotErrors.timeout
    if "duckdb" in data_options.db_name:
        if "io error" in fail_reason:
            return PlotErrors.i_o_full
        if "out of memory error" in fail_reason:
            return PlotErrors.oom
        if "code_" in fail_reason:
            return PlotErrors.not_available
    raise Exception(f"not handled {data_options}, {fail_reason}")


QUERY_LABEL = "Query (in the order of increasing rel. capture overhead)"


QUERY_DIFF_COLOS = ["#d6d6d6", "white"]


def generic_dump_legends(fig, ax, out_dir, handles, ncols):
    leg = ax.legend(
        handles=handles,
        prop=dict(size=7),
        ncols=ncols,
        labelspacing=0.2,
        handletextpad=0.3,
        columnspacing=0.8,
    )
    fig.canvas.draw()

    bbox = leg.get_window_extent()
    bbox = bbox.transformed(fig.dpi_scale_trans.inverted())
    fig.savefig(
        out_dir / "legends.pdf",
        bbox_inches=bbox,
    )
