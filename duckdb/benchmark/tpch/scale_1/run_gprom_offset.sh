
set -e
set -x

python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --db ../tpch_gprom_sf_01.db --config ../configs/config_gprom.json --dir gprom_params_default_restricted_keys_optimized --suff macos_gprom_offset_SMART --optimized --single_row_mode
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --db ../tpch_gprom_sf_01.db --config ../configs/config_gprom.json --dir gprom_params_default_restricted_keys_optimized --suff macos_gprom_offset_SMART --optimized --threads 2 --single_row_mode
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --db ../tpch_gprom_sf_01.db --config ../configs/config_gprom.json --dir gprom_params_default_restricted_keys_optimized --suff macos_gprom_offset_SMART --optimized --threads 4 --single_row_mode
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --db ../tpch_gprom_sf_01.db --config ../configs/config_gprom.json --dir gprom_params_default_restricted_keys_optimized --suff macos_gprom_offset_SMART --optimized --threads 8 --single_row_mode
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --db ../tpch_gprom_sf_01.db --config ../configs/config_gprom.json --dir gprom_params_default_restricted_keys_optimized --suff macos_gprom_offset_SMART --optimized --threads 10 --single_row_mode
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --db ../tpch_gprom_sf_01.db --config ../configs/config_gprom.json --dir gprom_params_default_restricted_keys_optimized --suff macos_gprom_offset_SMART --optimized --threads 12 --single_row_mode
