# Run GProM:
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_polynomial_duckdb_gprom --db ./microbench_polynomial_v04_base.db --config config_factorization_gprom.json --traceprov_fast

# Run TraceProv
python3 run.py --exe ../../../bld/bin/run_smokedduck --suff macos_polynomial_duckdb_traceprov --db ./microbench_polynomial_v04_base.db --config config_factorization.json --traceprov_fast

# Run SmokedDuck:
python3 run.py --exe ../../../bld/bin/run_smokedduck --suff macos_polynomial_duckdb_smokedduck --db ./microbench_polynomial_v04_base.db --config config_factorization.json --traceprov_fast --sd_mode old