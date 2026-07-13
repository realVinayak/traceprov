WITH temp_view_2 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0", F0_0.z AS "GROUP_0", F0_0."prov_skew__1__0__num__1000000_id" AS "prov_skew__1__0__num__1000000_id", dense_rank() OVER ( ORDER BY F0_0.z) AS _result_tid, row_number() OVER (PARTITION BY F0_0.z ORDER BY F0_0.z) AS _setprov_dup_count
FROM (
SELECT F0_0.id AS id, F0_0.z AS z, F0_0.val AS val, F0_0.id AS "prov_skew__1__0__num__1000000_id", (F0_0.id)::int8 AS _result_tid, 1 AS _setprov_dup_count, sum(F0_0.val) OVER (PARTITION BY F0_0.z) AS "AGGR_0"
FROM "skew_SKEW_VALUE_num_ROW_COUNT" F0_0) F0_0),
temp_view_1 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS "sum(val)", F0_0."GROUP_0" AS z, F0_0."prov_skew__1__0__num__1000000_id" AS "prov_skew__1__0__num__1000000_id", F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_2) F0_0),
temp_view_0 AS (
SELECT /*+ materialize */ F0_0."sum(val)" AS "sum(val)", F0_0.z AS z, F0_0."prov_skew__1__0__num__1000000_id" AS "prov_skew__1__0__num__1000000_id"
FROM (SELECT * FROM temp_view_1) F0_0)
SELECT F0_0."prov_skew__1__0__num__1000000_id" AS "prov_skew__1__0__num__1000000_id"
FROM (SELECT * FROM temp_view_0) F0_0;


