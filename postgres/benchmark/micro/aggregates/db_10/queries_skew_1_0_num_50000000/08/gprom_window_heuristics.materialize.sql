CREATE TEMP TABLE gprom_lineage AS (SELECT F0_0."prov_skew__1__0__num__50000000_id" AS "prov_skew__1__0__num__50000000_id"

FROM (

SELECT "prov_skew__1__0__num__50000000_id" AS "prov_skew__1__0__num__50000000_id", dense_rank() OVER ( ORDER BY F0_0."c" ASC NULLS LAST, F0_0."_result_tid") AS "_result_tid"

FROM (

SELECT F0_0."AGGR_0" AS "c", F0_0."prov_skew__1__0__num__50000000_id" AS "prov_skew__1__0__num__50000000_id", dense_rank() OVER ( ORDER BY F0_0."AGG_GB_ARG1") AS "_result_tid"

FROM (

SELECT 1 AS "AGG_GB_ARG0", F0_0."z" AS "AGG_GB_ARG1", F0_0."id" AS "prov_skew__1__0__num__50000000_id", count(1) OVER (PARTITION BY F0_0."z") AS "AGGR_0"

FROM "skew_1_0_num_50000000" F0_0) F0_0

ORDER BY "c" ASC NULLS LAST) F0_0) F0_0

WHERE (F0_0."_result_tid" <= 1));