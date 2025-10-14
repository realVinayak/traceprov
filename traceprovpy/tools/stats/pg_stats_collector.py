from typing import Any, NamedTuple
import psycopg2
from traceprovpy.tools.run_with_timeout import ConnectionParams


class PgStatsCollector(NamedTuple):

    version_sql = "SELECT version();"
    parameters_sql = "SHOW ALL;"
    stat_views = {
        "pg_stat_archiver",
        "pg_stat_bgwriter",
        "pg_stat_database",
        "pg_stat_database_conflicts",
        "pg_stat_user_tables",
        "pg_statio_user_tables",
        "pg_stat_user_indexes",
        "pg_statio_user_indexes",
    }

    def cursor_to_dict(self, cursor: psycopg2.extensions.cursor):
        results = cursor.fetchall()
        column_id_map = {
            name: idx
            for (idx, name) in list(
                enumerate([desc.name for desc in cursor.description])
            )
        }
        mapped_response = [
            {
                column_name: row[column_id]
                for (column_name, column_id) in column_id_map.items()
            }
            for row in results
        ]
        return mapped_response

    def fetch_stat_view(self, cursor: psycopg2.extensions.cursor, view: str):
        cursor.execute(f"select * from {view};")
        return self.cursor_to_dict(cursor)

    def collect_stats(self, connection_params: ConnectionParams) -> dict[str, Any]:
        connection = psycopg2.connect(
            database=connection_params.database,
            host=connection_params.host,
            user=connection_params.user,
            password=connection_params.password,
            port=connection_params.port,
        )

        cursor = connection.cursor()
        cursor.execute(PgStatsCollector.version_sql)
        stats_version = dict(version=cursor.fetchone()[0])

        cursor.execute(PgStatsCollector.parameters_sql)
        settings = cursor.fetchall()
        column_name_idx = {
            name: idx
            for (idx, name) in list(
                enumerate([desc.name for desc in cursor.description])
            )
        }

        stats_settings = {
            row[column_name_idx["name"]]: row[column_name_idx["setting"]]
            for row in settings
        }

        assert len(settings) == len(stats_settings)

        stat_views = {
            view: self.fetch_stat_view(cursor, view) for view in self.stat_views
        }

        all_stats = dict(
            version=stats_version, settings=stats_settings, stat_views=stat_views
        )
        cursor.close()
        connection.close()

        return all_stats


if __name__ == "__main__":
    stats_collector = PgStatsCollector()
    stats_collector.collect_stats(
        ConnectionParams(
            host="127.0.0.1",
            port="5432",
            user="postgres",
            password="postgres",
            database="postgres",
        )
    )
