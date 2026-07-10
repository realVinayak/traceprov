SELECT F0_0."prov_data__table__10__000__000_id" AS "prov_data__table__10__000__000_id"
FROM (
SELECT F0_0."AGGR_0" AS "AGGR_0", F1_0."prov_data__table__10__000__000_id" AS "prov_data__table__10__000__000_id"
FROM ((
SELECT min(F0_0.min_value) AS "AGGR_0", F0_0.group_number AS "GROUP_0"
FROM "data_table_10_000_000" F0_0
GROUP BY F0_0.group_number) F0_0 JOIN (
SELECT F0_0.group_number AS "_P_SIDE_GROUP_0", F0_0.id AS "prov_data__table__10__000__000_id"
FROM "data_table_10_000_000" F0_0) F1_0 ON ((F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0")))) F0_0
WHERE (F0_0."AGGR_0" <= :selectivity);