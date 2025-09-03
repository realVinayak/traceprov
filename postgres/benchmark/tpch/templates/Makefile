qgen_dir := ~/projects/tpch_scratch/tpch_ext/dbgen/
base_dir := ~/projects/prov/postgres/benchmark/tpch/templates/
seed_args = -r $(seed)

ifndef target_dir
$(error target_dir is not set)
endif

ifndef scale
$(error scale is not set)
endif

ifndef seed
ifndef default
$(error seed and default or not set)
endif
endif

ifdef seed
ifdef default
$(error both seed and default are set)
endif
endif


ifdef default
# if doing default, reset the seed args to be default
seed_args := -d
endif

queries:
	mkdir -p $(target_dir)
	mkdir -p $(target_dir)/base
	mkdir -p $(target_dir)/traceprov
	mkdir -p $(target_dir)/validate
	cd $(qgen_dir) && \
	for q in `seq 1 22`; do DSS_QUERY=$(base_dir)base/ ./qgen -s $(scale) $(seed_args) $$q > $(target_dir)/base/$$q.sql; done
	cd $(qgen_dir) && \
	for q in `seq 1 22`; do DSS_QUERY=$(base_dir)traceprov/ ./qgen -s $(scale) $(seed_args) $$q > $(target_dir)/traceprov/$$q.traceprov.sql; done
	cd $(qgen_dir) && \
	for q in `seq 1 22`; do DSS_QUERY=$(base_dir)validate/ ./qgen -s $(scale) $(seed_args) $$q > $(target_dir)/validate/$$q.validate.sql; done