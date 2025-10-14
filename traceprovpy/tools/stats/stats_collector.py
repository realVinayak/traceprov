from typing import Any, Literal, NamedTuple

from traceprovpy.tools.run_with_timeout import ConnectionParams
from traceprovpy.tools.stats.pg_stats_collector import PgStatsCollector

VENDORS = Literal["postgres"]


class StatsCollector(NamedTuple):
    def collect_stats(
        self, vendor: VENDORS, connection_params: ConnectionParams
    ) -> dict[str, Any]:
        if vendor == "postgres":
            return PgStatsCollector().collect_stats(connection_params)

        raise Exception("got unexpected vendor: ", vendor)
