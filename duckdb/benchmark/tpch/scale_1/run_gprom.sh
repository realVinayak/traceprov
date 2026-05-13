
set -e
set -x

python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ../tpch_01_versioned.db --config ../configs/config_gprom.json --gprom_config ./params_default_gprom/config_gprom_settings.json --threads 1 --optimized
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ../tpch_01_versioned.db --config ../configs/config_gprom.json --gprom_config ./params_default_gprom/config_gprom_settings.json --threads 2 --optimized
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ../tpch_01_versioned.db --config ../configs/config_gprom.json --gprom_config ./params_default_gprom/config_gprom_settings.json --threads 4 --optimized
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ../tpch_01_versioned.db --config ../configs/config_gprom.json --gprom_config ./params_default_gprom/config_gprom_settings.json --threads 8 --optimized
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ../tpch_01_versioned.db --config ../configs/config_gprom.json --gprom_config ./params_default_gprom/config_gprom_settings.json --threads 10 --optimized
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ../tpch_01_versioned.db --config ../configs/config_gprom.json --gprom_config ./params_default_gprom/config_gprom_settings.json --threads 12 --optimized
