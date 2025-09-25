WITH temp_view_1 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0", F0_0."z" AS "GROUP_0", F0_0."prov_skew__1__0__num__10000000_id" AS "prov_skew__1__0__num__10000000_id", dense_rank() OVER ( ORDER BY F0_0."z") AS "_result_tid", row_number() OVER (PARTITION BY F0_0."z" ORDER BY F0_0."z") AS "_setprov_dup_count"
FROM (
SELECT F0_0."id" AS "id", F0_0."z" AS "z", F0_0."val" AS "val", F0_0."id" AS "prov_skew__1__0__num__10000000_id", ROW_NUMBER() OVER () AS "_result_tid", 1 AS "_setprov_dup_count", min(F0_0."val") OVER (PARTITION BY F0_0."z") AS "AGGR_0"
FROM "skew_1_0_num_10000000" F0_0) F0_0),
temp_view_0 AS (
SELECT /*+ materialize */ F0_0."GROUP_0" AS "min_value", F0_0."AGGR_0" AS "count(z)", F0_0."prov_skew__1__0__num__10000000_id" AS "prov_skew__1__0__num__10000000_id", F0_0."_result_tid" AS "_result_tid", F0_0."_setprov_dup_count" AS "_setprov_dup_count"
FROM (
SELECT F0_0."AGGR_0" AS "AGGR_0", F0_0."AGGR_1" AS "AGGR_1", F0_0."min_value" AS "GROUP_0", F0_0."prov_skew__1__0__num__10000000_id" AS "prov_skew__1__0__num__10000000_id", dense_rank() OVER ( ORDER BY F0_0."min_value") AS "_result_tid", row_number() OVER (PARTITION BY F0_0."min_value" ORDER BY F0_0."min_value") AS "_setprov_dup_count"
FROM (
SELECT F0_0."AGGR_0" AS "min_value", F0_0."GROUP_0" AS "z", F0_0."prov_skew__1__0__num__10000000_id" AS "prov_skew__1__0__num__10000000_id", F0_0."_result_tid" AS "_result_tid", F0_0."_setprov_dup_count" AS "_setprov_dup_count", count((CASE  WHEN (1 = F0_0."_setprov_dup_count") THEN F0_0."GROUP_0" ELSE (NULL)::int8 END)) OVER (PARTITION BY F0_0."AGGR_0") AS "AGGR_0", count((CASE  WHEN (1 = F0_0."_setprov_dup_count") THEN F0_0."GROUP_0" ELSE (NULL)::int8 END)) OVER (PARTITION BY F0_0."AGGR_0") AS "AGGR_1"
FROM (SELECT * FROM temp_view_1) F0_0) F0_0) F0_0
WHERE (F0_0."AGGR_0" > 900))
SELECT F0_0."prov_skew__1__0__num__10000000_id" AS "prov_skew__1__0__num__10000000_id"
FROM (SELECT * FROM temp_view_0) F0_0;