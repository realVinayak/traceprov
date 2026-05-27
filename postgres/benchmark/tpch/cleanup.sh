#!/bin/bash
set -e
set -x

PG_DIR="/home/realv/projects/traceprov_clone/traceprov/postgres/benchmark/tpch"

git rm -r $PG_DIR/scale_1/gprom_params_default_keys
git rm -r $PG_DIR/scale_10/gprom_params_default_keys