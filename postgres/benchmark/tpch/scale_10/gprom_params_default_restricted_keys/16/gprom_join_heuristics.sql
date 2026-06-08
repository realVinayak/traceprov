
SELECT F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.p_brand AS p_brand, F0_0.p_type AS p_type, F0_0.p_size AS p_size
FROM (
SELECT F0_0."GROUP_0" AS p_brand, F0_0."GROUP_1" AS p_type, F0_0."GROUP_2" AS p_size, F0_0."AGGR_0" AS supplier_cnt, F1_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F1_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F1_0.prov_part_p__partkey AS prov_part_p__partkey
FROM ((
SELECT count(DISTINCT F0_0.ps_suppkey) AS "AGGR_0", F1_0.p_brand AS "GROUP_0", F1_0.p_type AS "GROUP_1", F1_0.p_size AS "GROUP_2"
FROM ((
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey
FROM partsupp F0_0) F0_0 CROSS JOIN (
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_brand AS p_brand, F0_0.p_type AS p_type, F0_0.p_size AS p_size
FROM part F0_0) F1_0), LATERAL (
SELECT (CASE  WHEN ((min((CASE  WHEN (F0_0.ps_suppkey <> F0_1.s_suppkey) THEN 2 WHEN (((F0_0.ps_suppkey) IS NULL) OR ((F0_1.s_suppkey) IS NULL)) THEN 1 ELSE 0 END))) IS NULL) THEN TRUE WHEN (min((CASE  WHEN (F0_0.ps_suppkey <> F0_1.s_suppkey) THEN 2 WHEN (((F0_0.ps_suppkey) IS NULL) OR ((F0_1.s_suppkey) IS NULL)) THEN 1 ELSE 0 END)) = 1) THEN (NULL)::bool ELSE (min((CASE  WHEN (F0_0.ps_suppkey <> F0_1.s_suppkey) THEN 2 WHEN (((F0_0.ps_suppkey) IS NULL) OR ((F0_1.s_suppkey) IS NULL)) THEN 1 ELSE 0 END)) = 2) END) AS "nesting_eval_1"
FROM (
SELECT F0_1.s_suppkey AS s_suppkey, F0_1.s_comment AS s_comment
FROM supplier F0_1) F0_1
WHERE (F0_1.s_comment LIKE '%Customer%Complaints%')) F2_0
WHERE (((F1_0.p_partkey = F0_0.ps_partkey) AND (F1_0.p_brand <> 'Brand#45')) AND (NOT ((((F1_0.p_type LIKE 'MEDIUM POLISHED%') AND F1_0.p_size IN (49, 14, 23, 45, 19, 3, 36, 9)) AND F2_0."nesting_eval_1"))))
GROUP BY F1_0.p_brand, F1_0.p_type, F1_0.p_size) F0_0 JOIN (
SELECT F0_0.p_brand AS "_P_SIDE_GROUP_0", F0_0.p_type AS "_P_SIDE_GROUP_1", F0_0.p_size AS "_P_SIDE_GROUP_2", F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey
FROM (
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.p_partkey AS p_partkey, F0_0.p_brand AS p_brand, F0_0.p_type AS p_type, F0_0.p_size AS p_size, F1_0."nesting_eval_1" AS "nesting_eval_1", F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey
FROM (
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey, F1_0.p_partkey AS p_partkey, F1_0.p_brand AS p_brand, F1_0.p_type AS p_type, F1_0.p_size AS p_size, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F1_0.prov_part_p__partkey AS prov_part_p__partkey
FROM ((
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey, F0_0.ps_partkey AS prov_partsupp_ps__partkey, F0_0.ps_suppkey AS prov_partsupp_ps__suppkey
FROM partsupp F0_0) F0_0 CROSS JOIN (
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_brand AS p_brand, F0_0.p_type AS p_type, F0_0.p_size AS p_size, F0_0.p_partkey AS prov_part_p__partkey
FROM part F0_0) F1_0)) F0_0, LATERAL (
SELECT (CASE  WHEN ((F0_1."nesting_eval_1") IS NULL) THEN TRUE WHEN (F0_1."nesting_eval_1" = 1) THEN (NULL)::bool ELSE (F0_1."nesting_eval_1" = 2) END) AS "nesting_eval_1"
FROM ((
SELECT min((CASE  WHEN (F0_0.ps_suppkey <> F0_1.s_suppkey) THEN 2 WHEN (((F0_0.ps_suppkey) IS NULL) OR ((F0_1.s_suppkey) IS NULL)) THEN 1 ELSE 0 END)) AS "nesting_eval_1"
FROM (
SELECT F0_1.s_suppkey AS s_suppkey, F0_1.s_comment AS s_comment
FROM supplier F0_1) F0_1
WHERE (F0_1.s_comment LIKE '%Customer%Complaints%')) F0_1 LEFT OUTER JOIN (
SELECT 0 AS __dummy_empty
FROM (
SELECT F0_1.s_comment AS s_comment
FROM supplier F0_1) F0_1
WHERE (F0_1.s_comment LIKE '%Customer%Complaints%')) F1_1 ON ((1 = 1)))) F1_0) F0_0
WHERE (((F0_0.p_partkey = F0_0.ps_partkey) AND (F0_0.p_brand <> 'Brand#45')) AND (NOT ((((F0_0.p_type LIKE 'MEDIUM POLISHED%') AND F0_0.p_size IN (49, 14, 23, 45, 19, 3, 36, 9)) AND F0_0."nesting_eval_1"))))) F1_0 ON (((F0_0."GROUP_2" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_2") AND ((F0_0."GROUP_1" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_1") AND (F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0")))))
ORDER BY supplier_cnt DESC NULLS LAST, p_brand ASC NULLS LAST, p_type ASC NULLS LAST, p_size ASC NULLS LAST) F0_0;


