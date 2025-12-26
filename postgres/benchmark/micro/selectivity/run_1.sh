#!/bin/bash

# python3 bench.py --sel_num_groups 10000 --sel_selectivity 0.01 --sel_mode post -u postgres -p postgres -db microbench_selectivity_10000 -suff local_test_duckdb_analyze_lz4_raw_v02 -tp_root ../../../ -t_root ./

# python3 bench.py --sel_num_groups 10000 --sel_selectivity 0.05 --sel_mode post -u postgres -p postgres -db microbench_selectivity_10000 -suff local_test_duckdb_analyze_lz4_raw_v02 -tp_root ../../../ -t_root ./

# python3 bench.py --sel_num_groups 10000 --sel_selectivity 0.10 --sel_mode post -u postgres -p postgres -db microbench_selectivity_10000 -suff local_test_duckdb_analyze_native_v02 -tp_root ../../../ -t_root ./

python3 bench.py --sel_num_groups 10000 --sel_selectivity 75 --sel_mode post -u postgres -p postgres -db microbench_selectivity_10000 -suff local_test_playground_v02 -tp_root ../../../ -t_root ./

# python3 bench.py --sel_num_groups 10000 --sel_selectivity 1.00 --sel_mode post -u postgres -p postgres -db microbench_selectivity_10000 -suff local_test_duckdb_analyze_lz4_raw_v02 -tp_root ../../../ -t_root ./

# python3 bench.py --sel_num_groups 10000 --sel_selectivity 5.00 --sel_mode post -u postgres -p postgres -db microbench_selectivity_10000 -suff local_test_duckdb_analyze_lz4_raw_v02 -tp_root ../../../ -t_root ./

# python3 bench.py --sel_num_groups 10000 --sel_selectivity 10.00 --sel_mode post -u postgres -p postgres -db microbench_selectivity_10000 -suff local_test_duckdb_analyze_lz4_raw_v02 -tp_root ../../../ -t_root ./

# python3 bench.py --sel_num_groups 10000 --sel_selectivity 50.00 --sel_mode post -u postgres -p postgres -db microbench_selectivity_10000 -suff local_test_duckdb_analyze_lz4_raw_v02 -tp_root ../../../ -t_root ./

# python3 bench.py --sel_num_groups 10000 --sel_selectivity 75.00 --sel_mode post -u postgres -p postgres -db microbench_selectivity_10000 -suff local_test_duckdb_analyze_lz4_raw_v02 -tp_root ../../../ -t_root ./
