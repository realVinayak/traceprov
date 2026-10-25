# Run TraceProv
set -e
set -x
CXX=clang++ CC=clang python3 bench_traceprov.py --config config_bench.json --db microbench_log_cost_v01_base --suff macos_traceprov_log_cost -tp_root ../../../ -t_root ./ -sd_lib /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -sd_include /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -H localhost -P 5460

# Run ProvSQL
set -e
set -x
CXX=clang++ CC=clang python3 bench_provsql.py --config config_bench.json --db microbench_log_cost_v02_provsql --suff macos_provsql_log_cost -tp_root ../../../ -t_root ./ -sd_lib /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -sd_include /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -H localhost -P 5460 -u uicdbgroup