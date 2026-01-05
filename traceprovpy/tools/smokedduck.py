from pathlib import Path
from traceprovpy.tools.benchmark import QuerySpec
import os
from time import perf_counter
import json

from traceprovpy.tools.run_with_timeout import SmokedDuckOptions

def load_json(file_path: str) -> dict:
    with open(file_path) as f:
        contents = json.loads(f.read())
    return contents

class SmokedDuckQuerySpec(QuerySpec):

    def run_packs(self, top_dir, get_run_options, benchmark):

        sd_options: SmokedDuckOptions = benchmark.sd_options
        assert sd_options is not None

        _pack = self.get_pack(top_dir, self.base, get_run_options)
        with open(_pack.file_path) as f:
            # enforce ascii (for now...)
            sql_contents = f.read().encode('ascii')
        
        file_path = '/tmp/duckdb_sql.sql'
        with open(file_path, 'wb') as f:
            f.write(sql_contents)

        pack = _pack._replace(file_path=file_path)

        assert os.path.exists(sd_options.driver_executable)

        # This dir acts as the scratch space for everything.
        # this way, files don't have to be individually added to .gitignore + makes things tidy.
        smokedduck_scratch_dir = "./smokedduck_scratch"
        assert os.path.exists(smokedduck_scratch_dir), "scratch space should be present!"
        duckdb_path = f"{pack.connection_params.database}.db"
        assert os.path.exists(f"{smokedduck_scratch_dir}/{duckdb_path}"), "DB not found!"

        repeat = pack.params.repeat + pack.params.throwaway
        start = perf_counter()
        settings_out = "duckdb_settings.json"
        assert os.system(f"cd {smokedduck_scratch_dir} && {sd_options.driver_executable} --db {duckdb_path} --threads 1 --repeat {repeat} --stats 0_%d_stats.json --profile 0_%d_plan.json --lineage --i {file_path} --settings {settings_out}") == 0
        end = perf_counter()

        # need to fetch all the results now (other than row aways.)
        stats = [load_json(f"{smokedduck_scratch_dir}/0_{i}_stats.json") for i in range(pack.params.throwaway, repeat)]
        profiles = [load_json(f"{smokedduck_scratch_dir}/0_{i}_plan.json") for i in range(pack.params.throwaway, repeat)]
        settings = load_json(f"{smokedduck_scratch_dir}/{settings_out}")

        results = [dict(stats=stats, profiles=profiles, settings=settings, total_time = (end - start) / repeat)]
        return results


        