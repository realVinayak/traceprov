set -e
set -x
# Run GProM all
CXX=clang++ CC=clang python3 ../bench_gprom.py --config ../configs/config_gprom.json -t_root ./ --dir gprom_params_default_restricted/ -db tpch_01_v01 --suff macos_gprom_all_18_20_21 -tp_root ../../../ -sd_lib /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -sd_include /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -H localhost -P 5460

# Run GProM single
CXX=clang++ CC=clang python3 ../bench_gprom.py --config ../configs/config_gprom.json -t_root ./ --dir gprom_params_default_restricted_keys/ -db tpch_01_v01 --suff macos_gprom_offset_18_20_21 -tp_root ../../../ -sd_lib /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -sd_include /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -H localhost -P 5460 --keys_mode

# Run ProvSQL all
CXX=clang++ CC=clang python3 ../bench_provsql.py --config ../config_provsql.json -t_root ./ -db tpch_01_v01 --suff macos_provsql -tp_root ../../../ -sd_lib /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -sd_include /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -H localhost -P 5462 -u uicdbgroup

# Run ProvSQL single
CXX=clang++ CC=clang python3 ../bench_provsql.py --config ../config_provsql.json -t_root ./ -db tpch_01_v01 --suff macos_provsql_offset -tp_root ../../../ -sd_lib /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -sd_include /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -H localhost -P 5462 -u uicdbgroup --no-bulk_derive

# Run TraceProv all
CXX=clang++ CC=clang python3 ../bench_parse_back_traceprov.py --config ../configs/config_bench.json -t_root ./ -db tpch_01_v01 --suff macos_traceprov_all_proc_0 -tp_root ../../../ -sd_lib /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -sd_include /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -H localhost -P 5460 --traceprov_fast --derive_config ../../../../duckdb/benchmark/tpch/query_layer_used.json

# Run TraceProv single
CXX=clang++ CC=clang python3 ../bench_parse_back_traceprov.py --config ../configs/config_bench.json -t_root ./ -db tpch_01_v01 --suff macos_traceprov_offset_proc_0 -tp_root ../../../ -sd_lib /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -sd_include /Users/uicdbgroup/vinny/git_experiments/duckdb_prebuilt -H localhost -P 5460 --traceprov_sample_fast --no-bulk_derive --derive_config ../../../../duckdb/benchmark/tpch/query_layer_used.json