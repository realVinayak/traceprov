WITH temp_view_1 AS (
SELECT /*+ materialize */ F0_0."id" AS "id", F0_0."z" AS "z", F0_0."val" AS "val", F0_0."id" AS "prov_skew__1__0__num__10000000_id", ROW_NUMBER() OVER () AS "_result_tid", 1 AS "_setprov_dup_count"
FROM "skew_1_0_num_10000000" F0_0),
temp_view_0 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0", F0_0."AGG_GB_ARG1" AS "GROUP_0", F0_0."prov_skew__1__0__num__10000000_id" AS "prov_skew__1__0__num__10000000_id", dense_rank() OVER ( ORDER BY F0_0."AGG_GB_ARG1") AS "_result_tid", row_number() OVER (PARTITION BY F0_0."AGG_GB_ARG1" ORDER BY F0_0."AGG_GB_ARG1") AS "_setprov_dup_count"
FROM (
SELECT 1 AS "AGG_GB_ARG0", F0_0."z" AS "AGG_GB_ARG1", F0_0."prov_skew__1__0__num__10000000_id" AS "prov_skew__1__0__num__10000000_id", F0_0."_result_tid" AS "_result_tid", F0_0."_setprov_dup_count" AS "_setprov_dup_count", count(1) OVER (PARTITION BY F0_0."z") AS "AGGR_0"
FROM (SELECT * FROM temp_view_1) F0_0) F0_0)
SELECT F0_0."prov_skew__1__0__num__10000000_id" AS "prov_skew__1__0__num__10000000_id"
FROM (
SELECT "c" AS "c", "z" AS "z", "prov_skew__1__0__num__10000000_id" AS "prov_skew__1__0__num__10000000_id", dense_rank() OVER ( ORDER BY F0_0."c" DESC NULLS LAST, F0_0."_result_tid") AS "_result_tid", "_setprov_dup_count" AS "_setprov_dup_count"
FROM (
SELECT F0_0."AGGR_0" AS "c", F0_0."GROUP_0" AS "z", F0_0."prov_skew__1__0__num__10000000_id" AS "prov_skew__1__0__num__10000000_id", F0_0."_result_tid" AS "_result_tid", F0_0."_setprov_dup_count" AS "_setprov_dup_count"
FROM (SELECT * FROM temp_view_0) F0_0
ORDER BY "c" DESC NULLS LAST) F0_0) F0_0
WHERE (F0_0."_result_tid" <= 10);