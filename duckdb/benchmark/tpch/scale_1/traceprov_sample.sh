#!/bin/bash

set -e
set -x

python3 ../run.py --exe ../../../bld/bin/run_smokedduck --sample_inference sample --db ../tpch_01_versioned.db --spec spec.json --base_root ./ --root ./ --config config_bench.json --query_layer_cfg ../query_layer_used.json --suff macos_sample_SMART --graph_dir ../traceprov_graphs --optimized
python3 ../run.py --exe ../../../bld/bin/run_smokedduck --sample_inference sample --db ../tpch_01_versioned.db --spec spec.json --base_root ./ --root ./ --config config_bench.json --query_layer_cfg ../query_layer_used.json --suff macos_sample_SMART --graph_dir ../traceprov_graphs --optimized --traceprov_use_merge_chunks
python3 ../run.py --exe ../../../bld/bin/run_smokedduck --sample_inference sample --db ../tpch_01_versioned.db --spec spec.json --base_root ./ --root ./ --config config_bench.json --query_layer_cfg ../query_layer_used.json --suff macos_sample_SMART --graph_dir ../traceprov_graphs --optimized --traceprov_use_compact
python3 ../run.py --exe ../../../bld/bin/run_smokedduck --sample_inference sample --db ../tpch_01_versioned.db --spec spec.json --base_root ./ --root ./ --config config_bench.json --query_layer_cfg ../query_layer_used.json --suff macos_sample_SMART --graph_dir ../traceprov_graphs --optimized --traceprov_use_merge_chunks --traceprov_use_compact

# partition queries
python3 ../run.py --exe ../../../bld/bin/run_smokedduck --sample_inference sample --db ../tpch_01_versioned.db --spec spec.json --base_root ./ --root ./ --config config_partition.json --query_layer_cfg ../query_layer_used.json --suff macos_sample_SMART --graph_dir ../traceprov_graphs --optimized --traceprov_use_partition_in_agg
python3 ../run.py --exe ../../../bld/bin/run_smokedduck --sample_inference sample --db ../tpch_01_versioned.db --spec spec.json --base_root ./ --root ./ --config config_partition.json --query_layer_cfg ../query_layer_used.json --suff macos_sample_SMART --graph_dir ../traceprov_graphs --optimized --traceprov_use_merge_chunks --traceprov_use_partition_in_agg
python3 ../run.py --exe ../../../bld/bin/run_smokedduck --sample_inference sample --db ../tpch_01_versioned.db --spec spec.json --base_root ./ --root ./ --config config_partition.json --query_layer_cfg ../query_layer_used.json --suff macos_sample_SMART --graph_dir ../traceprov_graphs --optimized --traceprov_use_compact --traceprov_use_partition_in_agg
python3 ../run.py --exe ../../../bld/bin/run_smokedduck --sample_inference sample --db ../tpch_01_versioned.db --spec spec.json --base_root ./ --root ./ --config config_partition.json --query_layer_cfg ../query_layer_used.json --suff macos_sample_SMART --graph_dir ../traceprov_graphs --optimized --traceprov_use_merge_chunks --traceprov_use_compact --traceprov_use_partition_in_agg
