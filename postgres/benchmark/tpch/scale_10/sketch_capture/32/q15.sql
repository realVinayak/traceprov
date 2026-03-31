SELECT fast_bit_or(F0."PROV_supplier1"::bit(32)) AS "PROV_supplier1", fast_bit_or(F0."PROV_lineitem1"::bit(32)) AS "PROV_lineitem1", fast_bit_or(F0."PROV_lineitem2"::bit(32)) AS "PROV_lineitem2"
FROM (
SELECT F0."s_suppkey" AS "s_suppkey", F0."s_name" AS "s_name", F0."s_address" AS "s_address", F0."s_phone" AS "s_phone", F0."total_revenue" AS "total_revenue", F0."PROV_supplier1" AS "PROV_supplier1", F0."PROV_lineitem1" AS "PROV_lineitem1", F0."PROV_lineitem2" AS "PROV_lineitem2"
FROM (
SELECT F0."s_suppkey" AS "s_suppkey", F0."s_name" AS "s_name", F0."s_address" AS "s_address", F0."s_nationkey" AS "s_nationkey", F0."s_phone" AS "s_phone", F0."s_acctbal" AS "s_acctbal", F0."s_comment" AS "s_comment", F1."supplier_no" AS "supplier_no", F1."total_revenue" AS "total_revenue", F0."PROV_supplier1" AS "PROV_supplier1", F1."PROV_lineitem1" AS "PROV_lineitem1", F1."PROV_lineitem2" AS "PROV_lineitem2"
FROM ((
SELECT F0."s_suppkey" AS "s_suppkey", F0."s_name" AS "s_name", F0."s_address" AS "s_address", F0."s_nationkey" AS "s_nationkey", F0."s_phone" AS "s_phone", F0."s_acctbal" AS "s_acctbal", F0."s_comment" AS "s_comment", set_bit(0::bit(32), binary_search_array_pos(ARRAY [1,3049,6157,9244,12316,15398,18511,21622,24821,28010,31244,34281,37386,40491,43630,46733,49976,53123,56098,59414,62587,65761,68809,71953,75065,78153,81329,84513,87560,90590,93882,96931,100001], s_suppkey) - 1,1 )  AS "PROV_supplier1"
FROM "supplier" AS F0) F0 CROSS JOIN (
SELECT F0."supplier_no" AS "supplier_no", F0."total_revenue" AS "total_revenue", F0."PROV_lineitem1" AS "PROV_lineitem1", F0."PROV_lineitem2" AS "PROV_lineitem2"
FROM (
SELECT F0."supplier_no" AS "supplier_no", F0."total_revenue" AS "total_revenue", F1."total_revenue2" AS "total_revenue2", F0."PROV_lineitem1" AS "PROV_lineitem1", F1."PROV_lineitem2" AS "PROV_lineitem2"
FROM ((
SELECT F0."GROUP_0" AS "supplier_no", F0."AGGR_0" AS "total_revenue", F0."PROV_lineitem1" AS "PROV_lineitem1"
FROM (
SELECT sum((F0."l_extendedprice" * (1 - F0."l_discount"))) AS "AGGR_0", F0."l_suppkey" AS "GROUP_0", fast_bit_or(F0."PROV_lineitem1"::bit(32)) AS "PROV_lineitem1"
FROM (
SELECT F0."l_orderkey" AS "l_orderkey", F0."l_partkey" AS "l_partkey", F0."l_suppkey" AS "l_suppkey", F0."l_linenumber" AS "l_linenumber", F0."l_quantity" AS "l_quantity", F0."l_extendedprice" AS "l_extendedprice", F0."l_discount" AS "l_discount", F0."l_tax" AS "l_tax", F0."l_returnflag" AS "l_returnflag", F0."l_linestatus" AS "l_linestatus", F0."l_shipdate" AS "l_shipdate", F0."l_commitdate" AS "l_commitdate", F0."l_receiptdate" AS "l_receiptdate", F0."l_shipinstruct" AS "l_shipinstruct", F0."l_shipmode" AS "l_shipmode", F0."l_comment" AS "l_comment", set_bit(0::bit(32), binary_search_array_pos(ARRAY [1,3239,6398,9486,12596,15741,18935,21896,25049,28193,31285,34236,37354,40739,43814,46966,50145,53299,56618,59824,62681,65708,68701,71924,75147,78244,81346,84419,87592,90889,93863,96940,100001], l_suppkey) - 1,1 )    AS "PROV_lineitem1"
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
SELECT sum((F0."l_extendedprice" * (1 - F0."l_discount"))) AS "AGGR_0", F0."l_suppkey" AS "GROUP_0", fast_bit_or(F0."PROV_lineitem2"::bit(32)) AS "PROV_lineitem2"
FROM (
SELECT F0."l_orderkey" AS "l_orderkey", F0."l_partkey" AS "l_partkey", F0."l_suppkey" AS "l_suppkey", F0."l_linenumber" AS "l_linenumber", F0."l_quantity" AS "l_quantity", F0."l_extendedprice" AS "l_extendedprice", F0."l_discount" AS "l_discount", F0."l_tax" AS "l_tax", F0."l_returnflag" AS "l_returnflag", F0."l_linestatus" AS "l_linestatus", F0."l_shipdate" AS "l_shipdate", F0."l_commitdate" AS "l_commitdate", F0."l_receiptdate" AS "l_receiptdate", F0."l_shipinstruct" AS "l_shipinstruct", F0."l_shipmode" AS "l_shipmode", F0."l_comment" AS "l_comment", set_bit(0::bit(32), binary_search_array_pos(ARRAY [1,3239,6398,9486,12596,15741,18935,21896,25049,28193,31285,34236,37354,40739,43814,46966,50145,53299,56618,59824,62681,65708,68701,71924,75147,78244,81346,84419,87592,90889,93863,96940,100001], l_suppkey) - 1,1 )   AS "PROV_lineitem2"
FROM "lineitem" AS F0) F0
WHERE ((F0."l_shipdate" >= '1996-12-01') and (F0."l_shipdate" < '1997-03-01'))
GROUP BY F0."l_suppkey") F0
) m1, 
(SELECT sum((F0."l_extendedprice" * (1 - F0."l_discount"))) AS "AGGR_0", F0."l_suppkey" AS "GROUP_0", fast_bit_or(F0."PROV_lineitem2"::bit(32)) AS "PROV_lineitem2"
FROM (
SELECT F0."l_orderkey" AS "l_orderkey", F0."l_partkey" AS "l_partkey", F0."l_suppkey" AS "l_suppkey", F0."l_linenumber" AS "l_linenumber", F0."l_quantity" AS "l_quantity", F0."l_extendedprice" AS "l_extendedprice", F0."l_discount" AS "l_discount", F0."l_tax" AS "l_tax", F0."l_returnflag" AS "l_returnflag", F0."l_linestatus" AS "l_linestatus", F0."l_shipdate" AS "l_shipdate", F0."l_commitdate" AS "l_commitdate", F0."l_receiptdate" AS "l_receiptdate", F0."l_shipinstruct" AS "l_shipinstruct", F0."l_shipmode" AS "l_shipmode", F0."l_comment" AS "l_comment", 
set_bit(0::bit(32), binary_search_array_pos(ARRAY [1,3239,6398,9486,12596,15741,18935,21896,25049,28193,31285,34236,37354,40739,43814,46966,50145,53299,56618,59824,62681,65708,68701,71924,75147,78244,81346,84419,87592,90889,93863,96940,100001], l_suppkey) - 1,1 )    AS "PROV_lineitem2"
FROM "lineitem" AS F0) F0
WHERE ((F0."l_shipdate" >= '1996-12-01') and (F0."l_shipdate" < '1997-03-01'))
GROUP BY F0."l_suppkey") m2
where m1."AGGR_0" = m2."AGGR_0"
)
F0) F1)) F0
WHERE (F0."total_revenue" = F0."total_revenue2")) F1)) F0
WHERE (F0."s_suppkey" = F0."supplier_no")) F0;
