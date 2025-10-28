SELECT F1_0."prov_data__table__100__000__000_id" AS "prov_data__table__100__000__000_id"
FROM ((
SELECT min(F0_0."min_value") AS "AGGR_0", F0_0."group_number" AS "GROUP_0"
FROM (
SELECT F0_0."min_value" AS "min_value", F0_0."negative_group_number" AS "negative_group_number", F0_0."group_number" AS "group_number"
FROM "data_table_100_000_000" F0_0) F0_0
WHERE (F0_0."negative_group_number" >= :selectivity)
GROUP BY F0_0."group_number") F0_0 JOIN (
SELECT F0_0."group_number" AS "_P_SIDE_GROUP_0", F0_0."prov_data__table__100__000__000_id" AS "prov_data__table__100__000__000_id"
FROM (
SELECT F0_0."negative_group_number" AS "negative_group_number", F0_0."group_number" AS "group_number", F0_0."id" AS "prov_data__table__100__000__000_id"
FROM "data_table_100_000_000" F0_0) F0_0
WHERE (F0_0."negative_group_number" >= :selectivity)) F1_0 ON ((F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0")));