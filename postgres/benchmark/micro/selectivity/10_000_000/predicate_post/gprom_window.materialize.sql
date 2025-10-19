CREATE TEMP TABLE gprom_lineage AS (WITH temp_view_0 AS (

SELECT /*+ materialize */ F0_0."AGGR_0" AS "min_over_group", F0_0."GROUP_0" AS "group_number", F0_0."prov_data__table__10__000__000_id" AS "prov_data__table__10__000__000_id", F0_0."_result_tid" AS "_result_tid", F0_0."_setprov_dup_count" AS "_setprov_dup_count"

FROM (

SELECT F0_0."AGGR_0" AS "AGGR_0", F0_0."AGGR_1" AS "AGGR_1", F0_0."group_number" AS "GROUP_0", F0_0."prov_data__table__10__000__000_id" AS "prov_data__table__10__000__000_id", dense_rank() OVER ( ORDER BY F0_0."group_number") AS "_result_tid", row_number() OVER (PARTITION BY F0_0."group_number" ORDER BY F0_0."group_number") AS "_setprov_dup_count"

FROM (

SELECT F0_0."id" AS "id", F0_0."min_value" AS "min_value", F0_0."negative_group_number" AS "negative_group_number", F0_0."group_number" AS "group_number", F0_0."id" AS "prov_data__table__10__000__000_id", ROW_NUMBER() OVER () AS "_result_tid", 1 AS "_setprov_dup_count", min(F0_0."min_value") OVER (PARTITION BY F0_0."group_number") AS "AGGR_0", min(F0_0."min_value") OVER (PARTITION BY F0_0."group_number") AS "AGGR_1"

FROM "data_table_10_000_000" F0_0) F0_0) F0_0

WHERE (F0_0."AGGR_0" <= :selectivity))

SELECT F0_0."prov_data__table__10__000__000_id" AS "prov_data__table__10__000__000_id"

FROM (SELECT * FROM temp_view_0) F0_0);