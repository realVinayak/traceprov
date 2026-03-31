SELECT fast_bit_or(F0."PROV_supplier1"::bit(64))::bit(64) AS "PROV_supplier1", fast_bit_or(F0."PROV_lineitem1"::bit(64))::bit(64) AS "PROV_lineitem1", fast_bit_or(F0."PROV_lineitem2"::bit(64))::bit(64) AS "PROV_lineitem2"
FROM (
SELECT F0."s_suppkey" AS "s_suppkey", F0."s_name" AS "s_name", F0."s_address" AS "s_address", F0."s_phone" AS "s_phone", F0."total_revenue" AS "total_revenue", F0."PROV_supplier1" AS "PROV_supplier1", F0."PROV_lineitem1" AS "PROV_lineitem1", F0."PROV_lineitem2" AS "PROV_lineitem2"
FROM (
SELECT F0."s_suppkey" AS "s_suppkey", F0."s_name" AS "s_name", F0."s_address" AS "s_address", F0."s_nationkey" AS "s_nationkey", F0."s_phone" AS "s_phone", F0."s_acctbal" AS "s_acctbal", F0."s_comment" AS "s_comment", F1."supplier_no" AS "supplier_no", F1."total_revenue" AS "total_revenue", F0."PROV_supplier1" AS "PROV_supplier1", F1."PROV_lineitem1" AS "PROV_lineitem1", F1."PROV_lineitem2" AS "PROV_lineitem2"
FROM ((
SELECT F0."s_suppkey" AS "s_suppkey", F0."s_name" AS "s_name", F0."s_address" AS "s_address", F0."s_nationkey" AS "s_nationkey", F0."s_phone" AS "s_phone", F0."s_acctbal" AS "s_acctbal", F0."s_comment" AS "s_comment", set_bit(0::bit(64), binary_search_array_pos(ARRAY [1,156,312,468,624,780,936,1092,1248,1404,1560,1716,1872,2028,2184,2340,2496,2652,2808,2964,3120,3276,3432,3588,3744,3900,4056,4212,4368,4524,4680,4836,4992,5149,5305,5462,5618,5775,5931,6088,6244,6401,6557,6714,6870,7027,7183,7340,7496,7653,7809,7966,8122,8279,8435,8592,8748,8905,9061,9218,9374,9531,9687,9844,10001], s_suppkey) - 1,1 )  AS "PROV_supplier1"
FROM "supplier" AS F0) F0 CROSS JOIN (
SELECT F0."supplier_no" AS "supplier_no", F0."total_revenue" AS "total_revenue", F0."PROV_lineitem1" AS "PROV_lineitem1", F0."PROV_lineitem2" AS "PROV_lineitem2"
FROM (
SELECT F0."supplier_no" AS "supplier_no", F0."total_revenue" AS "total_revenue", F1."total_revenue2" AS "total_revenue2", F0."PROV_lineitem1" AS "PROV_lineitem1", F1."PROV_lineitem2" AS "PROV_lineitem2"
FROM ((
SELECT F0."GROUP_0" AS "supplier_no", F0."AGGR_0" AS "total_revenue", F0."PROV_lineitem1" AS "PROV_lineitem1"
FROM (
SELECT sum((F0."l_extendedprice" * (1 - F0."l_discount"))) AS "AGGR_0", F0."l_suppkey" AS "GROUP_0", fast_bit_or(F0."PROV_lineitem1"::bit(64)) AS "PROV_lineitem1"
FROM (
SELECT F0."l_orderkey" AS "l_orderkey", F0."l_partkey" AS "l_partkey", F0."l_suppkey" AS "l_suppkey", F0."l_linenumber" AS "l_linenumber", F0."l_quantity" AS "l_quantity", F0."l_extendedprice" AS "l_extendedprice", F0."l_discount" AS "l_discount", F0."l_tax" AS "l_tax", F0."l_returnflag" AS "l_returnflag", F0."l_linestatus" AS "l_linestatus", F0."l_shipdate" AS "l_shipdate", F0."l_commitdate" AS "l_commitdate", F0."l_receiptdate" AS "l_receiptdate", F0."l_shipinstruct" AS "l_shipinstruct", F0."l_shipmode" AS "l_shipmode", F0."l_comment" AS "l_comment", set_bit(0::bit(64), binary_search_array_pos(ARRAY [1,157,313,469,627,782,938,1092,1249,1405,1563,1719,1875,2031,2188,2343,2499,2656,2812,2969,3125,3281,3437,3595,3751,3907,4064,4220,4377,4533,4689,4846,5001,5157,5314,5471,5627,5783,5939,6095,6252,6407,6563,6719,6876,7032,7188,7344,7500,7657,7813,7969,8125,8282,8438,8594,8750,8907,9063,9220,9376,9532,9688,9845,10001], l_suppkey) - 1,1 )   AS "PROV_lineitem1"
FROM "lineitem" AS F0) F0
WHERE ((F0."l_shipdate" >= '1996-12-01') and (F0."l_shipdate" < '1997-03-01'))
GROUP BY F0."l_suppkey") F0) F0 CROSS JOIN (
SELECT F0."AGGR_0" AS "total_revenue2", F0."PROV_lineitem2" AS "PROV_lineitem2"
FROM 
(
select m2."AGGR_0", m2."PROV_lineitem2" from 
(
SELECT max(F0."AGGR_0") AS "AGGR_0"
FROM (
SELECT sum((F0."l_extendedprice" * (1 - F0."l_discount"))) AS "AGGR_0", F0."l_suppkey" AS "GROUP_0", fast_bit_or(F0."PROV_lineitem2"::bit(64)) AS "PROV_lineitem2"
FROM (
SELECT F0."l_orderkey" AS "l_orderkey", F0."l_partkey" AS "l_partkey", F0."l_suppkey" AS "l_suppkey", F0."l_linenumber" AS "l_linenumber", F0."l_quantity" AS "l_quantity", F0."l_extendedprice" AS "l_extendedprice", F0."l_discount" AS "l_discount", F0."l_tax" AS "l_tax", F0."l_returnflag" AS "l_returnflag", F0."l_linestatus" AS "l_linestatus", F0."l_shipdate" AS "l_shipdate", F0."l_commitdate" AS "l_commitdate", F0."l_receiptdate" AS "l_receiptdate", F0."l_shipinstruct" AS "l_shipinstruct", F0."l_shipmode" AS "l_shipmode", F0."l_comment" AS "l_comment", set_bit(0::bit(64), binary_search_array_pos(ARRAY [1,157,313,469,627,782,938,1092,1249,1405,1563,1719,1875,2031,2188,2343,2499,2656,2812,2969,3125,3281,3437,3595,3751,3907,4064,4220,4377,4533,4689,4846,5001,5157,5314,5471,5627,5783,5939,6095,6252,6407,6563,6719,6876,7032,7188,7344,7500,7657,7813,7969,8125,8282,8438,8594,8750,8907,9063,9220,9376,9532,9688,9845,10001], l_suppkey) - 1,1 )   AS "PROV_lineitem2"
FROM "lineitem" AS F0) F0
WHERE ((F0."l_shipdate" >= '1996-12-01') and (F0."l_shipdate" < '1997-03-01'))
GROUP BY F0."l_suppkey") F0
) m1, 
(SELECT sum((F0."l_extendedprice" * (1 - F0."l_discount"))) AS "AGGR_0", F0."l_suppkey" AS "GROUP_0", fast_bit_or(F0."PROV_lineitem2"::bit(64)) AS "PROV_lineitem2"
FROM (
SELECT F0."l_orderkey" AS "l_orderkey", F0."l_partkey" AS "l_partkey", F0."l_suppkey" AS "l_suppkey", F0."l_linenumber" AS "l_linenumber", F0."l_quantity" AS "l_quantity", F0."l_extendedprice" AS "l_extendedprice", F0."l_discount" AS "l_discount", F0."l_tax" AS "l_tax", F0."l_returnflag" AS "l_returnflag", F0."l_linestatus" AS "l_linestatus", F0."l_shipdate" AS "l_shipdate", F0."l_commitdate" AS "l_commitdate", F0."l_receiptdate" AS "l_receiptdate", F0."l_shipinstruct" AS "l_shipinstruct", F0."l_shipmode" AS "l_shipmode", F0."l_comment" AS "l_comment", 
set_bit(0::bit(64), binary_search_array_pos(ARRAY [1,157,313,469,627,782,938,1092,1249,1405,1563,1719,1875,2031,2188,2343,2499,2656,2812,2969,3125,3281,3437,3595,3751,3907,4064,4220,4377,4533,4689,4846,5001,5157,5314,5471,5627,5783,5939,6095,6252,6407,6563,6719,6876,7032,7188,7344,7500,7657,7813,7969,8125,8282,8438,8594,8750,8907,9063,9220,9376,9532,9688,9845,10001], l_suppkey) - 1,1 )   AS "PROV_lineitem2"
FROM "lineitem" AS F0) F0
WHERE ((F0."l_shipdate" >= '1996-12-01') and (F0."l_shipdate" < '1997-03-01'))
GROUP BY F0."l_suppkey") m2
where m1."AGGR_0" = m2."AGGR_0"
)
F0) F1)) F0
WHERE (F0."total_revenue" = F0."total_revenue2")) F1)) F0
WHERE (F0."s_suppkey" = F0."supplier_no")) F0;
