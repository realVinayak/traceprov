
set -e
set -x

#python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ../tpch_10_versioned_backup.db --config ../configs/config_gprom.json --gprom_config ./params_default_gprom/config_gprom_settings.json --threads 3
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ../tpch_10_versioned_backup.db --config ../configs/config_gprom.json --gprom_config ./params_default_gprom/config_gprom_settings.json --threads 2
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ../tpch_10_versioned_backup.db --config ../configs/config_gprom.json --gprom_config ./params_default_gprom/config_gprom_settings.json --threads 4
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ../tpch_10_versioned_backup.db --config ../configs/config_gprom.json --gprom_config ./params_default_gprom/config_gprom_settings.json --threads 8
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ../tpch_10_versioned_backup.db --config ../configs/config_gprom.json --gprom_config ./params_default_gprom/config_gprom_settings.json --threads 10
python3 ../run_gprom.py --exe ../../../bld/bin/run_smokedduck --suff macos_gprom_SMART --db ../tpch_10_versioned_backup.db --config ../configs/config_gprom.json --gprom_config ./params_default_gprom/config_gprom_settings.json --threads 12
