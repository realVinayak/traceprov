SELECT F0_0."prov_skew__1__0__num__5000000_id" AS "prov_skew__1__0__num__5000000_id"
FROM (
SELECT F0_0."AGGR_0" AS "AGGR_0", F0_0."prov_skew__1__0__num__5000000_id" AS "prov_skew__1__0__num__5000000_id"
FROM (
SELECT F0_0."AGGR_0" AS "min_value", F0_0."z" AS "z", F0_0."prov_skew__1__0__num__5000000_id" AS "prov_skew__1__0__num__5000000_id", F0_0."_setprov_dup_count" AS "_setprov_dup_count", count((CASE  WHEN (1 = F0_0."_setprov_dup_count") THEN F0_0."z" ELSE (NULL)::int8 END)) OVER (PARTITION BY F0_0."AGGR_0") AS "AGGR_0", count((CASE  WHEN (1 = F0_0."_setprov_dup_count") THEN F0_0."z" ELSE (NULL)::int8 END)) OVER (PARTITION BY F0_0."AGGR_0") AS "AGGR_1"
FROM (
SELECT F0_0."z" AS "z", F0_0."prov_skew__1__0__num__5000000_id" AS "prov_skew__1__0__num__5000000_id", F0_0."AGGR_0" AS "AGGR_0", row_number() OVER (PARTITION BY F0_0."z" ORDER BY F0_0."z") AS "_setprov_dup_count"
FROM (
SELECT F0_0."z" AS "z", F0_0."val" AS "val", F0_0."id" AS "prov_skew__1__0__num__5000000_id", min(F0_0."val") OVER (PARTITION BY F0_0."z") AS "AGGR_0"
FROM "skew_1_0_num_5000000" F0_0) F0_0) F0_0) F0_0
WHERE (F0_0."AGGR_0" = F0_0."AGGR_1")) F0_0
WHERE (F0_0."AGGR_0" > 900);