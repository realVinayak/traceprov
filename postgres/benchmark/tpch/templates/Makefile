qgen_dir := ~/projects/tpch_scratch/tpch_ext/dbgen/
base_dir := ~/projects/prov/postgres/benchmark/tpch/templates/
seed_args = -r $(seed)

custom_val_list := (11.a, 11.b, 17.a, 17.b)

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
	rm -rf $(target_dir)/base
	rm -rf $(target_dir)/traceprov
	rm -rf $(target_dir)/validate
	mkdir -p $(target_dir)/base
	mkdir -p $(target_dir)/traceprov
	mkdir -p $(target_dir)/validate
	cd $(qgen_dir) && \
	for q in `seq 1 22`; do DSS_QUERY=$(base_dir)base/ ./qgen -s $(scale) $(seed_args) $$q > $(target_dir)/base/$$q.sql; done
	cd $(qgen_dir) && \
	for q in `seq 1 22`; do DSS_QUERY=$(base_dir)traceprov/ ./qgen -s $(scale) $(seed_args) $$q > $(target_dir)/traceprov/$$q.traceprov.sql; done
	cd $(qgen_dir) && \
	for q in `seq 1 22`; do DSS_QUERY=$(base_dir)validate/ ./qgen -s $(scale) $(seed_args) $$q > $(target_dir)/validate/$$q.validate.sql; done
	cd $(qgen_dir) && \
	DSS_QUERY=$(base_dir)validate/ ./qgen -s $(scale) $(seed_args) 11.a > $(target_dir)/validate/11.a.validate.sql;
	cd $(qgen_dir) && \
	DSS_QUERY=$(base_dir)validate/ ./qgen -s $(scale) $(seed_args) 11.b > $(target_dir)/validate/11.b.validate.sql;
	cd $(qgen_dir) && \
	DSS_QUERY=$(base_dir)validate/ ./qgen -s $(scale) $(seed_args) 17.a > $(target_dir)/validate/17.a.validate.sql;
	cd $(qgen_dir) && \
	DSS_QUERY=$(base_dir)validate/ ./qgen -s $(scale) $(seed_args) 17.b > $(target_dir)/validate/17.b.validate.sql;

examples:
	echo "make queries target_dir=~/projects/prov/postgres/benchmark/tpch/scale_1/params_5 scale=1 seed=1755708657"

%.sql:
# 	rm -rf $(target_dir)
# 	mkdir -p $(target_dir)
# 	mkdir -p $(target_dir)/base
# 	mkdir -p $(target_dir)/validate
# 	mkdir -p $(target_dir)/traceprov
	cd $(qgen_dir) && \
	DSS_QUERY=$(base_dir)base/ ./qgen -s $(scale) $(seed_args) $* > $(target_dir)/base/$*.sql;
	cd $(qgen_dir) && \
	DSS_QUERY=$(base_dir)traceprov/ ./qgen -s $(scale) $(seed_args) $* > $(target_dir)/traceprov/$*.traceprov.sql;
	cd $(qgen_dir) && \
	DSS_QUERY=$(base_dir)validate/ ./qgen -s $(scale) $(seed_args) $* > $(target_dir)/validate/$*.validate.sql;

no_limit: 2.no_limit.sql 3.no_limit.sql 10.no_limit.sql 18.no_limit.sql 21.no_limit.sql