
SELECT F1_0."prov_skew__1__0__num__1000000_id" AS "prov_skew__1__0__num__1000000_id", F0_0."GROUP_0" AS z
FROM ((
SELECT F0_0.z AS "GROUP_0"
FROM "skew_1_0_num_1000000" F0_0
GROUP BY F0_0.z) F0_0 JOIN (
SELECT F0_0.z AS "_P_SIDE_GROUP_0", F0_0.id AS "prov_skew__1__0__num__1000000_id"
FROM "skew_1_0_num_1000000" F0_0) F1_0 ON ((F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")));


