#!/bin/bash
set -e
set -x

python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_10.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 1
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_10.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 2
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_10.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 4
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_10.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 8
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_10.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 10
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_10.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 12

python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_100.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 1
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_100.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 2
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_100.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 4
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_100.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 8
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_100.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 10
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_100.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 12

python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_1000.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 1
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_1000.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 2
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_1000.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 4
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_1000.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 8
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_1000.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 10
python3 run_gprom.py --exe ../../../bld/bin/run_smokedduck --db /Users/uicdbgroup/vinny/git_experiments/traceprov_clone/traceprov/data/microbench_agg_02_zip/microbench_agg_02_1000.db --config config_bench.json --suff macos_gprom_SMART --optimized --threads 12
