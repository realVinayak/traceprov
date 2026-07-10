#!/bin/bash
set -e
set -x


rm -rf ./dump_scratch/
mkdir dump_scratch

PG_DIR="/home/realv/projects/traceprov_clone/traceprov/postgres/benchmark/tpch"
DUCKDB_DIR="/home/realv/projects/traceprov_clone/traceprov/duckdb/benchmark/tpch"

echo "USING DIR AS $PG_DIR and $DUCKDB_DIR."
ls $PG_DIR
ls $DUCKDB_DIR

# exit

# All Postgres

# All mode
python3 make_gprom.py --backend postgres -u postgres -p postgres --db tpch_01_v01 --sf 1 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_all --is_all
python3 make_gprom.py --backend postgres -u postgres -p postgres --db tpch_10_v02 --sf 10 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_all --is_all

# rm -rf $PG_DIR/scale_1/gprom_params_default_all
# mkdir $PG_DIR/scale_1/gprom_params_default_all
cp -r ./dump_scratch/gprom_all/postgres/1/* $PG_DIR/scale_1/gprom_params_default_all

# rm -rf $PG_DIR/scale_10/gprom_params_default_all
# mkdir $PG_DIR/scale_10/gprom_params_default_all
cp -r ./dump_scratch/gprom_all/postgres/10/* $PG_DIR/scale_10/gprom_params_default_all

# Restricted
python3 make_gprom.py --backend postgres -u postgres -p postgres --db tpch_01_v01 --sf 1 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_restricted
python3 make_gprom.py --backend postgres -u postgres -p postgres --db tpch_10_v02 --sf 10 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_restricted

# rm -rf $PG_DIR/scale_1/gprom_params_default_restricted
# mkdir $PG_DIR/scale_1/gprom_params_default_restricted
cp -r ./dump_scratch/gprom_restricted/postgres/1/* $PG_DIR/scale_1/gprom_params_default_restricted

# rm -rf $PG_DIR/scale_10/gprom_params_default_restricted
# mkdir $PG_DIR/scale_10/gprom_params_default_restricted
cp -r ./dump_scratch/gprom_restricted/postgres/10/* $PG_DIR/scale_10/gprom_params_default_restricted

# Restricted + keys
python3 make_gprom.py --backend postgres -u postgres -p postgres --db tpch_01_v01 --sf 1 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_restricted_keys --add_keys
python3 make_gprom.py --backend postgres -u postgres -p postgres --db tpch_10_v02 --sf 10 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_restricted_keys --add_keys

# rm -rf $PG_DIR/scale_1/gprom_params_default_restricted_keys
# rm -rf $PG_DIR/scale_1/gprom_params_default_keys
# mkdir $PG_DIR/scale_1/gprom_params_default_restricted_keys
cp -r ./dump_scratch/gprom_restricted_keys/postgres/1/* $PG_DIR/scale_1/gprom_params_default_restricted_keys

# rm -rf $PG_DIR/scale_10/gprom_params_default_restricted_keys
# rm -rf $PG_DIR/scale_10/gprom_params_default_keys
# mkdir $PG_DIR/scale_10/gprom_params_default_restricted_keys
cp -r ./dump_scratch/gprom_restricted_keys/postgres/10/* $PG_DIR/scale_10/gprom_params_default_restricted_keys

# All DuckDB

# All mode
# python3 make_gprom.py --backend duckdb --db tpch_01_v01.db --sf 1 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_all --is_all
# python3 make_gprom.py --backend duckdb --db ~/projects/traceprov_clone/traceprov/duckdb/benchmark/tpch/tpch_10_0.db --sf 10 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_all --is_all

# # rm -rf $DUCKDB_DIR/scale_1/gprom_params_default_all
# # mkdir $DUCKDB_DIR/scale_1/gprom_params_default_all
# cp -r ./dump_scratch/gprom_all/duckdb/1/* $DUCKDB_DIR/scale_1/gprom_params_default_all

# # rm -rf $DUCKDB_DIR/scale_10/gprom_params_default_all
# # mkdir $DUCKDB_DIR/scale_10/gprom_params_default_all
# cp -r ./dump_scratch/gprom_all/duckdb/10/* $DUCKDB_DIR/scale_10/gprom_params_default_all

# Restricted
python3 make_gprom.py --backend duckdb --db tpch_01_v01.db --sf 1 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_restricted
python3 make_gprom.py --backend duckdb --db ~/projects/traceprov_clone/traceprov/duckdb/benchmark/tpch/tpch_10_0.db --sf 10 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_restricted

# rm -rf $DUCKDB_DIR/scale_1/gprom_params_default_restricted
# mkdir $DUCKDB_DIR/scale_1/gprom_params_default_restricted
cp -r ./dump_scratch/gprom_restricted/duckdb/1/* $DUCKDB_DIR/scale_1/gprom_params_default_restricted

# rm -rf $DUCKDB_DIR/scale_10/gprom_params_default_restricted
# mkdir $DUCKDB_DIR/scale_10/gprom_params_default_restricted
cp -r ./dump_scratch/gprom_restricted/duckdb/10/* $DUCKDB_DIR/scale_10/gprom_params_default_restricted

# Restricted + Optimized
python3 make_gprom.py --backend duckdb --db tpch_01_v01.db --sf 1 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_restricted_optimized --optimized
python3 make_gprom.py --backend duckdb --db ~/projects/traceprov_clone/traceprov/duckdb/benchmark/tpch/tpch_10_0.db --sf 10 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_restricted_optimized --optimized

# rm -rf $DUCKDB_DIR/scale_1/gprom_params_default_restricted_optimized
# mkdir $DUCKDB_DIR/scale_1/gprom_params_default_restricted_optimized
cp -r ./dump_scratch/gprom_restricted_optimized/duckdb/1/* $DUCKDB_DIR/scale_1/gprom_params_default_restricted_optimized

# rm -rf $DUCKDB_DIR/scale_10/gprom_params_default_restricted_optimized
# mkdir $DUCKDB_DIR/scale_10/gprom_params_default_restricted_optimized
cp -r ./dump_scratch/gprom_restricted_optimized/duckdb/10/* $DUCKDB_DIR/scale_10/gprom_params_default_restricted_optimized

# Restricted + keys
python3 make_gprom.py --backend duckdb --db tpch_01_v01.db --sf 1 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_restricted_keys --add_keys
python3 make_gprom.py --backend duckdb --db ~/projects/traceprov_clone/traceprov/duckdb/benchmark/tpch/tpch_10_0.db --sf 10 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_restricted_keys --add_keys

# rm -rf $DUCKDB_DIR/scale_1/gprom_params_default_restricted_keys
# mkdir $DUCKDB_DIR/scale_1/gprom_params_default_restricted_keys
cp -r ./dump_scratch/gprom_restricted_keys/duckdb/1/* $DUCKDB_DIR/scale_1/gprom_params_default_restricted_keys

# rm -rf $DUCKDB_DIR/scale_10/gprom_params_default_restricted_keys
# mkdir $DUCKDB_DIR/scale_10/gprom_params_default_restricted_keys
cp -r ./dump_scratch/gprom_restricted_keys/duckdb/10/* $DUCKDB_DIR/scale_10/gprom_params_default_restricted_keys

# Restricted + keys + optimized
python3 make_gprom.py --backend duckdb --db tpch_01_v01.db --sf 1 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_restricted_keys_optimized --add_keys --optimized
python3 make_gprom.py --backend duckdb --db ~/projects/traceprov_clone/traceprov/duckdb/benchmark/tpch/tpch_10_0.db --sf 10 --source legacy_scale_1/params_default/extract_gprom/ --dest ./dump_scratch/gprom_restricted_keys_optimized --add_keys --optimized

# rm -rf $DUCKDB_DIR/scale_1/gprom_params_default_restricted_keys_optimized
# mkdir $DUCKDB_DIR/scale_1/gprom_params_default_restricted_keys_optimized
cp -r ./dump_scratch/gprom_restricted_keys_optimized/duckdb/1/* $DUCKDB_DIR/scale_1/gprom_params_default_restricted_keys_optimized

# rm -rf $DUCKDB_DIR/scale_10/gprom_params_default_restricted_keys_optimized
# mkdir $DUCKDB_DIR/scale_10/gprom_params_default_restricted_keys_optimized
cp -r ./dump_scratch/gprom_restricted_keys_optimized/duckdb/10/* $DUCKDB_DIR/scale_10/gprom_params_default_restricted_keys_optimized