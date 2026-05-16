#!/bin/bash
set -e
set -x


#python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ~/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_selectivity_10000_v02.db --config config_bench.json --num_groups 10_000 --optimized --threads 1
#python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ~/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_selectivity_10000_v02.db --config config_bench.json --num_groups 10_000 --optimized --threads 2
#python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ~/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_selectivity_10000_v02.db --config config_bench.json --num_groups 10_000 --optimized --threads 4
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ~/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_selectivity_10000_v02.db --config config_bench.json --num_groups 10_000 --optimized --threads 8
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ~/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_selectivity_10000_v02.db --config config_bench.json --num_groups 10_000 --optimized --threads 10
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ~/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_selectivity_10000_v02.db --config config_bench.json --num_groups 10_000 --optimized --threads 12


#python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ~/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_selectivity_10000_v02.db --config config_bench.json --num_groups 10_000 --threads 1
#python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ~/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_selectivity_10000_v02.db --config config_bench.json --num_groups 10_000 --threads 2
#python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ~/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_selectivity_10000_v02.db --config config_bench.json --num_groups 10_000 --threads 4
#python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ~/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_selectivity_10000_v02.db --config config_bench.json --num_groups 10_000 --threads 8
#python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ~/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_selectivity_10000_v02.db --config config_bench.json --num_groups 10_000 --threads 10
#python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ~/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_selectivity_10000_v02.db --config config_bench.json --num_groups 10_000 --threads 12
