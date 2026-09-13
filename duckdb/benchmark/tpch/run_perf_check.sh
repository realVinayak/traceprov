#!/bin/bash

set -e
set -x

CONFIG_VAR=../config_var.json
CONFIG_VAR_DUMP=../config_var_dump.json

CONFIG_VAR_BACKUP=$CONFIG_VAR.bak
CONFIG_VAR_DUMP_BACKUP=$CONFIG_VAR_DUMP.bak

for i in {1..22}; do

    cp $CONFIG_VAR $CONFIG_VAR_BACKUP
    sed "s/QID/$i/" $CONFIG_VAR_BACKUP > $CONFIG_VAR

    cp $CONFIG_VAR_DUMP $CONFIG_VAR_DUMP_BACKUP
    sed "s/QID/$i/" $CONFIG_VAR_DUMP_BACKUP > $CONFIG_VAR_DUMP

    echo "On Query: $i"
    # cat config_var.json
    # need to first run the version that just performs the dump.
    python3 ../run.py --exe ../../../bld/bin/run_smokedduck --spec spec.json --base_root ./ --root ./ --query_layer_cfg ../query_layer_used.json --graph_dir ../traceprov_graphs/ --config $CONFIG_VAR_DUMP --suff local_test_table_dump_$i --db $1  --threads $2 --mat_infer --dump_base_table
    
    # now run the first TraceProv,
    python3 ../run.py --exe ../../../bld/bin/run_smokedduck --spec spec.json --base_root ./ --root ./ --query_layer_cfg ../query_layer_used.json --graph_dir ../traceprov_graphs/ --config $CONFIG_VAR --suff local_test_variance_traceprov_run_SMART_$i --db $1  --threads $2

    # now run the first TraceProv,
    python3 ../run.py --exe ../../../bld/bin/run_smokedduck --spec spec.json --base_root ./ --root ./ --query_layer_cfg ../query_layer_used.json --graph_dir ../traceprov_graphs/ --config $CONFIG_VAR --suff local_test_variance_duckdb_run_SMART_$i --db $1  --threads $2 --no_use_table_def

    cp $CONFIG_VAR_BACKUP $CONFIG_VAR
    cp $CONFIG_VAR_DUMP_BACKUP $CONFIG_VAR_DUMP
done