WITH temp_view_2 AS (
SELECT /*+ materialize */ F0_0."id" AS "id", F0_0."z" AS "z", F0_0."val" AS "val", F0_0."id" AS "prov_skew__1__1__num__100000000_id"
FROM "skew_1_1_num_100000000" F0_0),
temp_view_1 AS (
SELECT /*+ materialize */ 1 AS "AGG_GB_ARG0", F0_0."z" AS "AGG_GB_ARG1", F0_0."prov_skew__1__1__num__100000000_id" AS "prov_skew__1__1__num__100000000_id"
FROM (SELECT * FROM temp_view_2) F0_0),
temp_view_0 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0", F0_0."GROUP_0" AS "GROUP_0", F1_0."prov_skew__1__1__num__100000000_id" AS "prov_skew__1__1__num__100000000_id"
FROM ((
SELECT count(1) AS "AGGR_0", F0_0."z" AS "GROUP_0"
FROM "skew_1_1_num_100000000" F0_0
GROUP BY F0_0."z") F0_0 JOIN (
SELECT F0_0."AGG_GB_ARG1" AS "_P_SIDE_GROUP_0", F0_0."prov_skew__1__1__num__100000000_id" AS "prov_skew__1__1__num__100000000_id"
FROM (SELECT * FROM temp_view_1) F0_0) F1_0 ON ((F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0"))))
SELECT F0_0."prov_skew__1__1__num__100000000_id" AS "prov_skew__1__1__num__100000000_id"
FROM (
SELECT F0_0."AGGR_0" AS "c", F0_0."GROUP_0" AS "z", F0_0."prov_skew__1__1__num__100000000_id" AS "prov_skew__1__1__num__100000000_id"
FROM (SELECT * FROM temp_view_0) F0_0
ORDER BY "c" DESC NULLS LAST
LIMIT 10) F0_0;