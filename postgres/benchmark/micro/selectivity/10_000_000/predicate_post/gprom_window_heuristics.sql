SELECT F0_0."prov_data__table__10__000__000_id" AS "prov_data__table__10__000__000_id"
FROM (
SELECT min(F0_0.min_value) OVER (PARTITION BY F0_0.group_number) AS "AGGR_0", F0_0.id AS "prov_data__table__10__000__000_id"
FROM "data_table_10_000_000" F0_0) F0_0
WHERE (F0_0."AGGR_0" <= :selectivity);