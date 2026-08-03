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
        if self.db_name in ['postgres']:
            return "postgres"
        assert False, f"Got db name: {self.db_name}"

    def true_sf(self):
        sf_split = self.sf.split("_")
        return int(sf_split[-1])

def get_options_split(parsed):
    db = Path(parsed.db).name
    print(db)
    db = db.replace(".db", '')
    assert db.count("_") == 4
    splitted = db.split("_")
    assert splitted[0] == 'data'
    sf = f"{splitted[1]}_{splitted[2]}"
    return DataOptions(sf=sf, db_name=splitted[3], mode=splitted[4])

class SystemLabels:
    base = "Base"
    traceprov = "TraceProv"
    gprom = "GProM"
    smokedduck = "SmokedDuck"
    muller = "Müller"


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
    SystemLabels.base: "tab:blue",
    SystemLabels.traceprov: "tab:orange",
    SystemLabels.gprom: "tab:green",
    SystemLabels.smokedduck: "tab:red",
    SystemLabels.muller: "tab:purple"
}

class PlotErrors:
    not_available = "N/A"
    timeout = "Timeout"
    i_o_full = "I/O Full"
    oom = "OOM"

ERROR_PATCH_MAPPING = [(PlotErrors.not_available, 'oo'), (PlotErrors.timeout, "///"), (PlotErrors.i_o_full, '|||'), (PlotErrors.oom, "xx")]

ERROR_HATCHES = {
    er[0]: er[1]
    for er in ERROR_PATCH_MAPPING
}