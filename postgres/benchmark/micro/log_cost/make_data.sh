# Create data (without ProvSQL)
python3 make_data.py --config config_bench.json --db microbench_log_cost_v01_base
# Create ProvSQL data
python3 make_data.py --config config_bench.json --db microbench_log_cost_v01_provsql --provsql
