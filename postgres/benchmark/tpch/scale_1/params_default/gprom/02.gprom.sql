--- SQL OUT --- ON 2025-09-10T14:11:35 

-- [1m[37m[40mERROR [0m[31m(query_operator_model_checker.c:482) [0mAttribute <ps_partkey> appears more than once in

-- [0m[30m[43mJoin[0m [((ps_supplycost = min_ps_suppcost) AND (ps_partkey = ps_partkey))]
--   [0m[30m[43mTableAccess[0m [partsupp]
--   [0m[30m[43mProjection[0m [AGGR_0 GROUP_0 ]
--     [0m[30m[43mAggregation[0m [(min(ps_supplycost))] GROUP BY [(ps_partkey)]
--       [0m[30m[43mSelection[0m [((((s_suppkey = ps_suppkey) AND (s_nationkey = n_nationkey)) AND (n_regionkey = r_regionkey)) AND (r_name = 'EUROPE'))]
--         [0m[30m[43mCrossProduct[0m []
--           [0m[30m[43mCrossProduct[0m []
--             [0m[30m[43mCrossProduct[0m []
--               [0m[30m[43mTableAccess[0m [partsupp]
--               [0m[30m[43mTableAccess[0m [supplier]
--             [0m[30m[43mTableAccess[0m [nation]
--           [0m[30m[43mTableAccess[0m [region]

WITH temp_view_0 AS (
SELECT F0_0."p_partkey" AS "p_partkey", F0_0."p_name" AS "p_name", F0_0."p_mfgr" AS "p_mfgr", F0_0."p_brand" AS "p_brand", F0_0."p_type" AS "p_type", F0_0."p_size" AS "p_size", F0_0."p_container" AS "p_container", F0_0."p_retailprice" AS "p_retailprice", F0_0."p_comment" AS "p_comment", F0_0."p_partkey" AS "prov_part_p__partkey"
FROM "part" F0_0),
temp_view_1 AS (
SELECT F0_0."s_suppkey" AS "s_suppkey", F0_0."s_name" AS "s_name", F0_0."s_address" AS "s_address", F0_0."s_nationkey" AS "s_nationkey", F0_0."s_phone" AS "s_phone", F0_0."s_acctbal" AS "s_acctbal", F0_0."s_comment" AS "s_comment", F0_0."s_suppkey" AS "prov_supplier_s__suppkey"
FROM "supplier" F0_0),
temp_view_2 AS (
SELECT F0_0."n_nationkey" AS "n_nationkey", F0_0."n_name" AS "n_name", F0_0."n_regionkey" AS "n_regionkey", F0_0."n_comment" AS "n_comment", F0_0."n_nationkey" AS "prov_nation_n__nationkey"
FROM "nation" F0_0),
temp_view_3 AS (
SELECT F0_0."r_regionkey" AS "r_regionkey", F0_0."r_name" AS "r_name", F0_0."r_comment" AS "r_comment", F0_0."r_regionkey" AS "prov_region_r__regionkey"
FROM "region" F0_0),
temp_view_4 AS (
SELECT F0_0."ps_partkey" AS "ps_partkey", F0_0."ps_suppkey" AS "ps_suppkey", F0_0."ps_availqty" AS "ps_availqty", F0_0."ps_supplycost" AS "ps_supplycost", F0_0."ps_comment" AS "ps_comment", F0_0."ps_partkey" AS "prov_partsupp_ps__partkey", F0_0."ps_suppkey" AS "prov_partsupp_ps__suppkey"
FROM "partsupp" F0_0),
temp_view_6 AS (
SELECT F0_0."ps_partkey" AS "ps_partkey", F0_0."ps_suppkey" AS "ps_suppkey", F0_0."ps_availqty" AS "ps_availqty", F0_0."ps_supplycost" AS "ps_supplycost", F0_0."ps_comment" AS "ps_comment", F0_0."ps_partkey" AS "prov_partsupp_1_ps__partkey", F0_0."ps_suppkey" AS "prov_partsupp_1_ps__suppkey"
FROM "partsupp" F0_0),
temp_view_7 AS (
SELECT F0_0."s_suppkey" AS "s_suppkey", F0_0."s_name" AS "s_name", F0_0."s_address" AS "s_address", F0_0."s_nationkey" AS "s_nationkey", F0_0."s_phone" AS "s_phone", F0_0."s_acctbal" AS "s_acctbal", F0_0."s_comment" AS "s_comment", F0_0."s_suppkey" AS "prov_supplier_1_s__suppkey"
FROM "supplier" F0_0),
temp_view_8 AS (
SELECT F0_0."n_nationkey" AS "n_nationkey", F0_0."n_name" AS "n_name", F0_0."n_regionkey" AS "n_regionkey", F0_0."n_comment" AS "n_comment", F0_0."n_nationkey" AS "prov_nation_1_n__nationkey"
FROM "nation" F0_0),
temp_view_9 AS (
SELECT F0_0."r_regionkey" AS "r_regionkey", F0_0."r_name" AS "r_name", F0_0."r_comment" AS "r_comment", F0_0."r_regionkey" AS "prov_region_1_r__regionkey"
FROM "region" F0_0),
temp_view_5 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0", F0_0."GROUP_0" AS "GROUP_0", F1_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F1_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F1_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F1_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey", F1_0."prov_region_1_r__regionkey" AS "prov_region_1_r__regionkey"
FROM ((
SELECT min(F0_0."ps_supplycost") AS "AGGR_0", F0_0."ps_partkey" AS "GROUP_0"
FROM ((("partsupp" F0_0 CROSS JOIN "supplier" F1_0) CROSS JOIN "nation" F2_0) CROSS JOIN "region" F3_0)
WHERE ((((F1_0."s_suppkey" = F0_0."ps_suppkey") AND (F1_0."s_nationkey" = F2_0."n_nationkey")) AND (F2_0."n_regionkey" = F3_0."r_regionkey")) AND (F3_0."r_name" = 'EUROPE'))
GROUP BY F0_0."ps_partkey") F0_0 JOIN (
SELECT F0_0."ps_partkey" AS "_P_SIDE_GROUP_0", F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F0_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F0_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey", F0_0."prov_region_1_r__regionkey" AS "prov_region_1_r__regionkey"
FROM (
SELECT F0_0."ps_partkey" AS "ps_partkey", F0_0."ps_suppkey" AS "ps_suppkey", F0_0."ps_availqty" AS "ps_availqty", F0_0."ps_supplycost" AS "ps_supplycost", F0_0."ps_comment" AS "ps_comment", F0_0."s_suppkey" AS "s_suppkey", F0_0."s_name" AS "s_name", F0_0."s_address" AS "s_address", F0_0."s_nationkey" AS "s_nationkey", F0_0."s_phone" AS "s_phone", F0_0."s_acctbal" AS "s_acctbal", F0_0."s_comment" AS "s_comment", F0_0."n_nationkey" AS "n_nationkey", F0_0."n_name" AS "n_name", F0_0."n_regionkey" AS "n_regionkey", F0_0."n_comment" AS "n_comment", F1_0."r_regionkey" AS "r_regionkey", F1_0."r_name" AS "r_name", F1_0."r_comment" AS "r_comment", F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F0_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F0_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey", F1_0."prov_region_1_r__regionkey" AS "prov_region_1_r__regionkey"
FROM ((
SELECT F0_0."ps_partkey" AS "ps_partkey", F0_0."ps_suppkey" AS "ps_suppkey", F0_0."ps_availqty" AS "ps_availqty", F0_0."ps_supplycost" AS "ps_supplycost", F0_0."ps_comment" AS "ps_comment", F0_0."s_suppkey" AS "s_suppkey", F0_0."s_name" AS "s_name", F0_0."s_address" AS "s_address", F0_0."s_nationkey" AS "s_nationkey", F0_0."s_phone" AS "s_phone", F0_0."s_acctbal" AS "s_acctbal", F0_0."s_comment" AS "s_comment", F1_0."n_nationkey" AS "n_nationkey", F1_0."n_name" AS "n_name", F1_0."n_regionkey" AS "n_regionkey", F1_0."n_comment" AS "n_comment", F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F0_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F1_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey"
FROM ((
SELECT F0_0."ps_partkey" AS "ps_partkey", F0_0."ps_suppkey" AS "ps_suppkey", F0_0."ps_availqty" AS "ps_availqty", F0_0."ps_supplycost" AS "ps_supplycost", F0_0."ps_comment" AS "ps_comment", F1_0."s_suppkey" AS "s_suppkey", F1_0."s_name" AS "s_name", F1_0."s_address" AS "s_address", F1_0."s_nationkey" AS "s_nationkey", F1_0."s_phone" AS "s_phone", F1_0."s_acctbal" AS "s_acctbal", F1_0."s_comment" AS "s_comment", F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F1_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey"
FROM ((SELECT * FROM temp_view_6) F0_0 CROSS JOIN (SELECT * FROM temp_view_7) F1_0)) F0_0 CROSS JOIN (SELECT * FROM temp_view_8) F1_0)) F0_0 CROSS JOIN (SELECT * FROM temp_view_9) F1_0)) F0_0
WHERE ((((F0_0."s_suppkey" = F0_0."ps_suppkey") AND (F0_0."s_nationkey" = F0_0."n_nationkey")) AND (F0_0."n_regionkey" = F0_0."r_regionkey")) AND (F0_0."r_name" = 'EUROPE'))) F1_0 ON ((F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0"))))
SELECT F0_0."s_acctbal" AS "s_acctbal", F0_0."s_name" AS "s_name", F0_0."n_name" AS "n_name", F0_0."p_partkey" AS "p_partkey", F0_0."p_mfgr" AS "p_mfgr", F0_0."s_address" AS "s_address", F0_0."s_phone" AS "s_phone", F0_0."s_comment" AS "s_comment", F0_0."prov_part_p__partkey" AS "prov_part_p__partkey", F0_0."prov_supplier_s__suppkey" AS "prov_supplier_s__suppkey", F0_0."prov_nation_n__nationkey" AS "prov_nation_n__nationkey", F0_0."prov_region_r__regionkey" AS "prov_region_r__regionkey", F0_0."prov_partsupp_ps__partkey" AS "prov_partsupp_ps__partkey", F0_0."prov_partsupp_ps__suppkey" AS "prov_partsupp_ps__suppkey", F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F0_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F0_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey", F0_0."prov_region_1_r__regionkey" AS "prov_region_1_r__regionkey"
FROM (
SELECT F0_0."p_partkey" AS "p_partkey", F0_0."p_name" AS "p_name", F0_0."p_mfgr" AS "p_mfgr", F0_0."p_brand" AS "p_brand", F0_0."p_type" AS "p_type", F0_0."p_size" AS "p_size", F0_0."p_container" AS "p_container", F0_0."p_retailprice" AS "p_retailprice", F0_0."p_comment" AS "p_comment", F0_0."s_suppkey" AS "s_suppkey", F0_0."s_name" AS "s_name", F0_0."s_address" AS "s_address", F0_0."s_nationkey" AS "s_nationkey", F0_0."s_phone" AS "s_phone", F0_0."s_acctbal" AS "s_acctbal", F0_0."s_comment" AS "s_comment", F0_0."n_nationkey" AS "n_nationkey", F0_0."n_name" AS "n_name", F0_0."n_regionkey" AS "n_regionkey", F0_0."n_comment" AS "n_comment", F0_0."r_regionkey" AS "r_regionkey", F0_0."r_name" AS "r_name", F0_0."r_comment" AS "r_comment", F1_0."ps_partkey" AS "ps_partkey", F1_0."ps_suppkey" AS "ps_suppkey", F1_0."ps_availqty" AS "ps_availqty", F1_0."ps_supplycost" AS "ps_supplycost", F1_0."ps_comment" AS "ps_comment", F1_0."min_ps_suppcost" AS "min_ps_suppcost", F1_0."ps_partkey1" AS "ps_partkey1", F0_0."prov_part_p__partkey" AS "prov_part_p__partkey", F0_0."prov_supplier_s__suppkey" AS "prov_supplier_s__suppkey", F0_0."prov_nation_n__nationkey" AS "prov_nation_n__nationkey", F0_0."prov_region_r__regionkey" AS "prov_region_r__regionkey", F1_0."prov_partsupp_ps__partkey" AS "prov_partsupp_ps__partkey", F1_0."prov_partsupp_ps__suppkey" AS "prov_partsupp_ps__suppkey", F1_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F1_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F1_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F1_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey", F1_0."prov_region_1_r__regionkey" AS "prov_region_1_r__regionkey"
FROM ((
SELECT F0_0."p_partkey" AS "p_partkey", F0_0."p_name" AS "p_name", F0_0."p_mfgr" AS "p_mfgr", F0_0."p_brand" AS "p_brand", F0_0."p_type" AS "p_type", F0_0."p_size" AS "p_size", F0_0."p_container" AS "p_container", F0_0."p_retailprice" AS "p_retailprice", F0_0."p_comment" AS "p_comment", F0_0."s_suppkey" AS "s_suppkey", F0_0."s_name" AS "s_name", F0_0."s_address" AS "s_address", F0_0."s_nationkey" AS "s_nationkey", F0_0."s_phone" AS "s_phone", F0_0."s_acctbal" AS "s_acctbal", F0_0."s_comment" AS "s_comment", F0_0."n_nationkey" AS "n_nationkey", F0_0."n_name" AS "n_name", F0_0."n_regionkey" AS "n_regionkey", F0_0."n_comment" AS "n_comment", F1_0."r_regionkey" AS "r_regionkey", F1_0."r_name" AS "r_name", F1_0."r_comment" AS "r_comment", F0_0."prov_part_p__partkey" AS "prov_part_p__partkey", F0_0."prov_supplier_s__suppkey" AS "prov_supplier_s__suppkey", F0_0."prov_nation_n__nationkey" AS "prov_nation_n__nationkey", F1_0."prov_region_r__regionkey" AS "prov_region_r__regionkey"
FROM ((
SELECT F0_0."p_partkey" AS "p_partkey", F0_0."p_name" AS "p_name", F0_0."p_mfgr" AS "p_mfgr", F0_0."p_brand" AS "p_brand", F0_0."p_type" AS "p_type", F0_0."p_size" AS "p_size", F0_0."p_container" AS "p_container", F0_0."p_retailprice" AS "p_retailprice", F0_0."p_comment" AS "p_comment", F0_0."s_suppkey" AS "s_suppkey", F0_0."s_name" AS "s_name", F0_0."s_address" AS "s_address", F0_0."s_nationkey" AS "s_nationkey", F0_0."s_phone" AS "s_phone", F0_0."s_acctbal" AS "s_acctbal", F0_0."s_comment" AS "s_comment", F1_0."n_nationkey" AS "n_nationkey", F1_0."n_name" AS "n_name", F1_0."n_regionkey" AS "n_regionkey", F1_0."n_comment" AS "n_comment", F0_0."prov_part_p__partkey" AS "prov_part_p__partkey", F0_0."prov_supplier_s__suppkey" AS "prov_supplier_s__suppkey", F1_0."prov_nation_n__nationkey" AS "prov_nation_n__nationkey"
FROM ((
SELECT F0_0."p_partkey" AS "p_partkey", F0_0."p_name" AS "p_name", F0_0."p_mfgr" AS "p_mfgr", F0_0."p_brand" AS "p_brand", F0_0."p_type" AS "p_type", F0_0."p_size" AS "p_size", F0_0."p_container" AS "p_container", F0_0."p_retailprice" AS "p_retailprice", F0_0."p_comment" AS "p_comment", F1_0."s_suppkey" AS "s_suppkey", F1_0."s_name" AS "s_name", F1_0."s_address" AS "s_address", F1_0."s_nationkey" AS "s_nationkey", F1_0."s_phone" AS "s_phone", F1_0."s_acctbal" AS "s_acctbal", F1_0."s_comment" AS "s_comment", F0_0."prov_part_p__partkey" AS "prov_part_p__partkey", F1_0."prov_supplier_s__suppkey" AS "prov_supplier_s__suppkey"
FROM ((SELECT * FROM temp_view_0) F0_0 CROSS JOIN (SELECT * FROM temp_view_1) F1_0)) F0_0 CROSS JOIN (SELECT * FROM temp_view_2) F1_0)) F0_0 CROSS JOIN (SELECT * FROM temp_view_3) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0."ps_partkey" AS "ps_partkey", F0_0."ps_suppkey" AS "ps_suppkey", F0_0."ps_availqty" AS "ps_availqty", F0_0."ps_supplycost" AS "ps_supplycost", F0_0."ps_comment" AS "ps_comment", F1_0."min_ps_suppcost" AS "min_ps_suppcost", F1_0."ps_partkey" AS "ps_partkey1", F0_0."prov_partsupp_ps__partkey" AS "prov_partsupp_ps__partkey", F0_0."prov_partsupp_ps__suppkey" AS "prov_partsupp_ps__suppkey", F1_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F1_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F1_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F1_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey", F1_0."prov_region_1_r__regionkey" AS "prov_region_1_r__regionkey"
FROM ((SELECT * FROM temp_view_4) F0_0 JOIN (
SELECT F0_0."AGGR_0" AS "min_ps_suppcost", F0_0."GROUP_0" AS "ps_partkey", F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F0_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F0_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey", F0_0."prov_region_1_r__regionkey" AS "prov_region_1_r__regionkey"
FROM (SELECT * FROM temp_view_5) F0_0) F1_0 ON (((F0_0."ps_supplycost" = F1_0."min_ps_suppcost") AND (F1_0."ps_partkey" = F0_0."ps_partkey"))))) F1_0)) F0_0
WHERE (((((((F0_0."p_partkey" = F0_0."ps_partkey") AND (F0_0."s_suppkey" = F0_0."ps_suppkey")) AND (F0_0."p_size" = 15)) AND (F0_0."p_type" LIKE '%BRASS')) AND (F0_0."s_nationkey" = F0_0."n_nationkey")) AND (F0_0."n_regionkey" = F0_0."r_regionkey")) AND (F0_0."r_name" = 'EUROPE'))
ORDER BY "s_acctbal" DESC NULLS LAST, "n_name" ASC NULLS LAST, "s_name" ASC NULLS LAST, "p_partkey" ASC NULLS LAST
LIMIT 100;