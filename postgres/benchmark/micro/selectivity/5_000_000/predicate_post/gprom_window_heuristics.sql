SELECT F0_0."prov_data__table__5__000__000_id" AS "prov_data__table__5__000__000_id"
FROM (
SELECT F0_0."AGGR_0" AS "AGGR_0", F0_0."prov_data__table__5__000__000_id" AS "prov_data__table__5__000__000_id"
FROM (
SELECT F0_0."min_value" AS "min_value", F0_0."group_number" AS "group_number", F0_0."id" AS "prov_data__table__5__000__000_id", min(F0_0."min_value") OVER (PARTITION BY F0_0."group_number") AS "AGGR_0", min(F0_0."min_value") OVER (PARTITION BY F0_0."group_number") AS "AGGR_1"
FROM "data_table_5_000_000" F0_0) F0_0
WHERE (F0_0."AGGR_0" = F0_0."AGGR_1")) F0_0
WHERE (F0_0."AGGR_0" <= :selectivity);