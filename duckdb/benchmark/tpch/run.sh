#!/bin/bash
#

set -e
set -x

OPT_OPTION=""
METHOD="hugw_mem"
OPT_STR="no_optimized"
if [ "$OPTIMIZED" = "true" ]; then
OPT_OPTION="--optimized"
OPT_STR="optimized"
fi

SUFF="aws_bench_${METHOD}_${OPT_STR}"


if [ "$VALIDATE" = "true" ]; then
python3 ../run.py --db $1 --exe ../../../bld/bin/run_smokedduck --suff local_test_0 --validate $OPT_OPTION --spec spec.json --config config_validate.json --base_root ~/traceprov/postgres/benchmark/tpch/scale_$2/params_default/ --root ./
else
python3 ../run.py --db $1 --exe ../../../bld/bin/run_smokedduck --suff $SUFF $OPT_OPTION --spec spec.json --config config_bench.json --base_root ~/traceprov/postgres/benchmark/tpch/scale_$2/params_default/ --root ./
fi
