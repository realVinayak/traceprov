# Run GProM
set -e
set -x
CXX=clang++ CC=clang python3 bench.py --config config_factorization.json --mode gprom --db microbench_polynomial_v04_base --suff macos_polynomial_gprom_v06 -tp_root ../../../ -t_root ./ -sd_lib /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -sd_include /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -P 5460 -u uicdbgroup

# Run ProvSQL
set -e
set -x
CXX=clang++ CC=clang python3 bench.py --config config_factorization.json --mode provsql --db microbench_polynomial_v05_provsql --suff macos_polynomial_provsql_v07 -tp_root ../../../ -t_root ./ -sd_lib /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -sd_include /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -P 5460 -u uicdbgroup

# Run TraceProv
set -e
set -x
CXX=clang++ CC=clang python3 bench.py --config config_factorization.json --mode traceprov --db microbench_polynomial_v04_base --suff macos_polynomial_traceprov_v06 -tp_root ../../../ -t_root ./ -sd_lib /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -sd_include /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -P 5460 -u uicdbgroup