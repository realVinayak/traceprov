
set -e
set -x

python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff local_test --db ../tpch_gprom_sf_01.db --config ../configs/config_gprom.json --dir gprom_params_default_restricted_optimized --suff macos_gprom_SMART --optimized
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff local_test --db ../tpch_gprom_sf_01.db --config ../configs/config_gprom.json --dir gprom_params_default_restricted_optimized --suff macos_gprom_SMART --optimized --threads 2
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff local_test --db ../tpch_gprom_sf_01.db --config ../configs/config_gprom.json --dir gprom_params_default_restricted_optimized --suff macos_gprom_SMART --optimized --threads 4
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff local_test --db ../tpch_gprom_sf_01.db --config ../configs/config_gprom.json --dir gprom_params_default_restricted_optimized --suff macos_gprom_SMART --optimized --threads 8
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff local_test --db ../tpch_gprom_sf_01.db --config ../configs/config_gprom.json --dir gprom_params_default_restricted_optimized --suff macos_gprom_SMART --optimized --threads 10
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff local_test --db ../tpch_gprom_sf_01.db --config ../configs/config_gprom.json --dir gprom_params_default_restricted_optimized --suff macos_gprom_SMART --optimized --threads 12
