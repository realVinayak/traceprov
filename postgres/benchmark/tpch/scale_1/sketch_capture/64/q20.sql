SELECT fast_bit_or(F0."PROV_lineitem1"::bit(10000))::bit(10000) AS "PROV_lineitem1"
FROM (
SELECT F0."s_name" AS "s_name", F0."s_address" AS "s_address",F0."PROV_lineitem1" AS "PROV_lineitem1"
FROM (
SELECT F0."s_name" AS "s_name", F0."s_address" AS "s_address", F0."PROV_lineitem1" AS "PROV_lineitem1"
FROM (
SELECT F0."s_suppkey" AS "s_suppkey", F0."s_name" AS "s_name", F0."s_address" AS "s_address", F0."s_nationkey" AS "s_nationkey", F0."s_phone" AS "s_phone", F0."s_acctbal" AS "s_acctbal", F0."s_comment" AS "s_comment", F0."n_nationkey" AS "n_nationkey", F0."n_name" AS "n_name", F0."n_regionkey" AS "n_regionkey", F0."n_comment" AS "n_comment", F1."ps_suppkey" AS "ps_suppkey", F1."PROV_lineitem1" AS "PROV_lineitem1"
FROM ((
SELECT F0."s_suppkey" AS "s_suppkey", F0."s_name" AS "s_name", F0."s_address" AS "s_address", F0."s_nationkey" AS "s_nationkey", F0."s_phone" AS "s_phone", F0."s_acctbal" AS "s_acctbal", F0."s_comment" AS "s_comment", F1."n_nationkey" AS "n_nationkey", F1."n_name" AS "n_name", F1."n_regionkey" AS "n_regionkey", F1."n_comment" AS "n_comment"
FROM ((
SELECT F0."s_suppkey" AS "s_suppkey", F0."s_name" AS "s_name", F0."s_address" AS "s_address", F0."s_nationkey" AS "s_nationkey", F0."s_phone" AS "s_phone", F0."s_acctbal" AS "s_acctbal", F0."s_comment" AS "s_comment"
FROM "supplier" AS F0) F0 CROSS JOIN "nation" AS F1)) F0 CROSS JOIN (
SELECT F0."ps_suppkey" AS "ps_suppkey", F0."PROV_lineitem1" AS "PROV_lineitem1"
FROM (
SELECT F0."ps_partkey" AS "ps_partkey", F0."ps_suppkey" AS "ps_suppkey", F0."ps_availqty" AS "ps_availqty", F0."ps_supplycost" AS "ps_supplycost", F0."ps_comment" AS "ps_comment", F0."p_partkey" AS "p_partkey", F1."l_partkey" AS "l_partkey", F1."l_suppkey" AS "l_suppkey", F1."sum_lq" AS "sum_lq", F1."PROV_lineitem1" AS "PROV_lineitem1"
FROM ((
SELECT F0."ps_partkey" AS "ps_partkey", F0."ps_suppkey" AS "ps_suppkey", F0."ps_availqty" AS "ps_availqty", F0."ps_supplycost" AS "ps_supplycost", F0."ps_comment" AS "ps_comment", F1."p_partkey" AS "p_partkey"
FROM ((
SELECT F0."ps_partkey" AS "ps_partkey", F0."ps_suppkey" AS "ps_suppkey", F0."ps_availqty" AS "ps_availqty", F0."ps_supplycost" AS "ps_supplycost", F0."ps_comment" AS "ps_comment"
FROM "partsupp" AS F0) F0 CROSS JOIN (
SELECT F0."p_partkey" AS "p_partkey"
FROM (
SELECT F0."p_partkey" AS "p_partkey", F0."p_name" AS "p_name", F0."p_mfgr" AS "p_mfgr", F0."p_brand" AS "p_brand", F0."p_type" AS "p_type", F0."p_size" AS "p_size", F0."p_container" AS "p_container", F0."p_retailprice" AS "p_retailprice", F0."p_comment" AS "p_comment"
FROM "part" AS F0) F0
WHERE (F0."p_name" LIKE 'green%')) F1)) F0 CROSS JOIN (
SELECT F0."GROUP_0" AS "l_partkey", F0."GROUP_1" AS "l_suppkey", (0.500000 * F0."AGGR_0") AS "sum_lq", F0."PROV_lineitem1" AS "PROV_lineitem1"
FROM (
SELECT sum(F0."l_quantity") AS "AGGR_0", F0."l_partkey" AS "GROUP_0", F0."l_suppkey" AS "GROUP_1", set_bits(F0."PROV_lineitem1")::bit(10000) AS "PROV_lineitem1"
FROM (
SELECT F0."l_orderkey" AS "l_orderkey", F0."l_partkey" AS "l_partkey", F0."l_suppkey" AS "l_suppkey", F0."l_linenumber" AS "l_linenumber", F0."l_quantity" AS "l_quantity", F0."l_extendedprice" AS "l_extendedprice", F0."l_discount" AS "l_discount", F0."l_tax" AS "l_tax", F0."l_returnflag" AS "l_returnflag", F0."l_linestatus" AS "l_linestatus", F0."l_shipdate" AS "l_shipdate", F0."l_commitdate" AS "l_commitdate", F0."l_receiptdate" AS "l_receiptdate", F0."l_shipinstruct" AS "l_shipinstruct", F0."l_shipmode" AS "l_shipmode", F0."l_comment" AS "l_comment", binary_search_array_pos(ARRAY [1,93476,186977,281445,375301,468644,562278,656032,749568,843686,937314,1031428,1125348,1219109,1312711,1406405,1500738,1594501,1687907,1781831,1874917,1968608,2062599,2156544,2250438,2344551,2438022,2531590,2626021,2719781,2813156,2907173,3000961,3094656,3188294,3282214,3375589,3469190,3562148,3656160,3749957,3843812,3937667,4031488,4125286,4219271,4312550,4406308,4500354,4593958,4687457,4781347,4875014,4968578,5062566,5156515,5250567,5344039,5437926,5532198,5625987,5719717,5813189,5906370,6000001], l_orderkey) - 1 AS "PROV_lineitem1"
FROM "lineitem" AS F0) F0
WHERE ((F0."l_shipdate" >= '1993-01-01') and (F0."l_shipdate" < '1994-01-01'))
GROUP BY F0."l_partkey", F0."l_suppkey") F0) F1)) F0
WHERE ((((F0."l_partkey" = F0."ps_partkey") and (F0."l_suppkey" = F0."ps_suppkey")) and (F0."ps_availqty" > F0."sum_lq")) and (F0."ps_partkey" = F0."p_partkey"))) F1)) F0
WHERE (((F0."s_suppkey" = F0."ps_suppkey") and (F0."s_nationkey" = F0."n_nationkey")) and (F0."n_name" = 'CHINA'))) F0 order by s_name) F0;
