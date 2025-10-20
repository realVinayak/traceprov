SELECT F0_0."prov_skew__1__0__num__5000000_id" AS "prov_skew__1__0__num__5000000_id"
FROM (
SELECT F0_0."AGGR_0" AS "AGGR_0", F1_0."prov_skew__1__0__num__5000000_id" AS "prov_skew__1__0__num__5000000_id"
FROM ((
SELECT count(F0_0."GROUP_0") AS "AGGR_0", count(F0_0."GROUP_0") AS "AGGR_1", F0_0."AGGR_0" AS "GROUP_0"
FROM (
SELECT min(F0_0."val") AS "AGGR_0", F0_0."z" AS "GROUP_0"
FROM "skew_1_0_num_5000000" F0_0
GROUP BY F0_0."z") F0_0
GROUP BY F0_0."AGGR_0") F0_0 JOIN (
SELECT F0_0."AGGR_0" AS "_P_SIDE_GROUP_0", F1_0."prov_skew__1__0__num__5000000_id" AS "prov_skew__1__0__num__5000000_id"
FROM ((
SELECT min(F0_0."val") AS "AGGR_0", F0_0."z" AS "GROUP_0"
FROM "skew_1_0_num_5000000" F0_0
GROUP BY F0_0."z") F0_0 JOIN (
SELECT F0_0."z" AS "_P_SIDE_GROUP_0", F0_0."id" AS "prov_skew__1__0__num__5000000_id"
FROM "skew_1_0_num_5000000" F0_0) F1_0 ON ((F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0")))) F1_0 ON ((F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0")))) F0_0
WHERE (F0_0."AGGR_0" > 900);