# Run TraceProv
python3 run.py --exe ../../../bld/bin/run_smokedduck --suff macos_traceprov_log_cost --db microbench_log_cost_traceprov.db --config ../../../../postgres/benchmark/micro/log_cost/config_bench.json --traceprov_fast

# Recompile TraceProv but set TRACEPROV_USE_MMEM_PAGE to 0.
python3 run.py --exe ../../../bld/bin/run_smokedduck --suff macos_traceprov_log_cost_file --db microbench_log_cost_traceprov.db --config ../../../../postgres/benchmark/micro/log_cost/config_bench.json --traceprov_fast

# Run SmokedDuck
python3 run.py --exe ../../../bld/bin/run_smokedduck --suff macos_sd_log_cost --db microbench_log_cost_sd.db --config ../../../../postgres/benchmark/micro/log_cost/config_bench.json --sd_mode old