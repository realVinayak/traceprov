WITH temp_view_2 AS (
SELECT /*+ materialize */ F0_0.id AS id, F0_0.min_value AS min_value, F0_0.negative_group_number AS negative_group_number, F0_0.group_number AS group_number, F0_0.id AS "prov_data__table__50__000__000_id", (F0_0.id)::int8 AS _result_tid, 1 AS _setprov_dup_count
FROM "data_table_50_000_000" F0_0),
temp_view_1 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS min_over_group, F0_0."GROUP_0" AS group_number, F0_0."prov_data__table__50__000__000_id" AS "prov_data__table__50__000__000_id", F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT F0_0."AGGR_0" AS "AGGR_0", F0_0."AGGR_1" AS "AGGR_1", F0_0."GROUP_0" AS "GROUP_0", F1_0."prov_data__table__50__000__000_id" AS "prov_data__table__50__000__000_id", dense_rank() OVER ( ORDER BY F0_0."GROUP_0") AS _result_tid, row_number() OVER (PARTITION BY F0_0."GROUP_0" ORDER BY F0_0."GROUP_0") AS _setprov_dup_count
FROM ((
SELECT min(F0_0.min_value) AS "AGGR_0", min(F0_0.min_value) AS "AGGR_1", F0_0.group_number AS "GROUP_0"
FROM "data_table_50_000_000" F0_0
GROUP BY F0_0.group_number) F0_0 JOIN (
SELECT F0_0.group_number AS "_P_SIDE_GROUP_0", F0_0."prov_data__table__50__000__000_id" AS "prov_data__table__50__000__000_id"
FROM (SELECT * FROM temp_view_2) F0_0) F1_0 ON ((F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0")))) F0_0
WHERE (F0_0."AGGR_0" <= :selectivity)),
temp_view_0 AS (
SELECT /*+ materialize */ F0_0.min_over_group AS min_over_group, F0_0.group_number AS group_number, F0_0."prov_data__table__50__000__000_id" AS "prov_data__table__50__000__000_id"
FROM (SELECT * FROM temp_view_1) F0_0)
SELECT F0_0."prov_data__table__50__000__000_id" AS "prov_data__table__50__000__000_id"
FROM (SELECT * FROM temp_view_0) F0_0;