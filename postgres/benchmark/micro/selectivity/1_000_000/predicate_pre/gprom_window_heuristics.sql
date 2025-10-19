SELECT F0_0."prov_data__table__1__000__000_id" AS "prov_data__table__1__000__000_id"
FROM (
SELECT F0_0."min_value" AS "min_value", F0_0."negative_group_number" AS "negative_group_number", F0_0."group_number" AS "group_number", F0_0."id" AS "prov_data__table__1__000__000_id"
FROM "data_table_1_000_000" F0_0) F0_0
WHERE (F0_0."negative_group_number" >= :selectivity);