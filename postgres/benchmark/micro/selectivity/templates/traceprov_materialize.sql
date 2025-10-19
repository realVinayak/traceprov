CREATE TEMP TABLE traceprov_lineage_selectivity AS
SELECT * FROM traceprov_infer_selectivity_bench(1, 0, 0);