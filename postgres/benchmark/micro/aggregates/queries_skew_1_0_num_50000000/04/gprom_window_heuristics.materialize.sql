CREATE TEMP TABLE gprom_lineage AS (SELECT F0_0."prov_skew__1__0__num__50000000_id" AS "prov_skew__1__0__num__50000000_id"

FROM (

SELECT F0_0."AGGR_0" AS "AGGR_0", F0_0."prov_skew__1__0__num__50000000_id" AS "prov_skew__1__0__num__50000000_id"

FROM (

SELECT F0_0."z" AS "z", F0_0."val" AS "val", F0_0."id" AS "prov_skew__1__0__num__50000000_id", avg(F0_0."val") OVER (PARTITION BY F0_0."z") AS "AGGR_0", avg(F0_0."val") OVER (PARTITION BY F0_0."z") AS "AGGR_1"

FROM "skew_1_0_num_50000000" F0_0) F0_0

WHERE (F0_0."AGGR_0" = F0_0."AGGR_1")) F0_0

WHERE (F0_0."AGGR_0" > 50));