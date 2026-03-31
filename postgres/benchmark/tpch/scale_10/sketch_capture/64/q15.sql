SELECT fast_bit_or(F0."PROV_supplier1"::bit(64)) AS "PROV_supplier1", fast_bit_or(F0."PROV_lineitem1"::bit(64)) AS "PROV_lineitem1", fast_bit_or(F0."PROV_lineitem2"::bit(64)) AS "PROV_lineitem2"
FROM (
SELECT F0."s_suppkey" AS "s_suppkey", F0."s_name" AS "s_name", F0."s_address" AS "s_address", F0."s_phone" AS "s_phone", F0."total_revenue" AS "total_revenue", F0."PROV_supplier1" AS "PROV_supplier1", F0."PROV_lineitem1" AS "PROV_lineitem1", F0."PROV_lineitem2" AS "PROV_lineitem2"
FROM (
SELECT F0."s_suppkey" AS "s_suppkey", F0."s_name" AS "s_name", F0."s_address" AS "s_address", F0."s_nationkey" AS "s_nationkey", F0."s_phone" AS "s_phone", F0."s_acctbal" AS "s_acctbal", F0."s_comment" AS "s_comment", F1."supplier_no" AS "supplier_no", F1."total_revenue" AS "total_revenue", F0."PROV_supplier1" AS "PROV_supplier1", F1."PROV_lineitem1" AS "PROV_lineitem1", F1."PROV_lineitem2" AS "PROV_lineitem2"
FROM ((
SELECT F0."s_suppkey" AS "s_suppkey", F0."s_name" AS "s_name", F0."s_address" AS "s_address", F0."s_nationkey" AS "s_nationkey", F0."s_phone" AS "s_phone", F0."s_acctbal" AS "s_acctbal", F0."s_comment" AS "s_comment", set_bit(0::bit(64), binary_search_array_pos(ARRAY [1,1564,3117,4843,6403,7966,9504,11037,12513,14105,15669,17190,18657,20217,21915,23433,24947,26526,28062,29628,31147,32702,34296,35816,37471,38961,40438,41916,43538,45287,46782,48312,49967,51503,53104,54647,56154,57663,59409,60857,62479,64082,65656,67195,68708,70350,71936,73542,75119,76713,78276,79887,81350,82889,84371,85971,87580,89132,90670,92249,93838,95347,96856,98407,100001], s_suppkey) - 1,1 )  AS "PROV_supplier1"
FROM "supplier" AS F0) F0 CROSS JOIN (
SELECT F0."supplier_no" AS "supplier_no", F0."total_revenue" AS "total_revenue", F0."PROV_lineitem1" AS "PROV_lineitem1", F0."PROV_lineitem2" AS "PROV_lineitem2"
FROM (
SELECT F0."supplier_no" AS "supplier_no", F0."total_revenue" AS "total_revenue", F1."total_revenue2" AS "total_revenue2", F0."PROV_lineitem1" AS "PROV_lineitem1", F1."PROV_lineitem2" AS "PROV_lineitem2"
FROM ((
SELECT F0."GROUP_0" AS "supplier_no", F0."AGGR_0" AS "total_revenue", F0."PROV_lineitem1" AS "PROV_lineitem1"
FROM (
SELECT sum((F0."l_extendedprice" * (1 - F0."l_discount"))) AS "AGGR_0", F0."l_suppkey" AS "GROUP_0", fast_bit_or(F0."PROV_lineitem1"::bit(64)) AS "PROV_lineitem1"
FROM (
SELECT F0."l_orderkey" AS "l_orderkey", F0."l_partkey" AS "l_partkey", F0."l_suppkey" AS "l_suppkey", F0."l_linenumber" AS "l_linenumber", F0."l_quantity" AS "l_quantity", F0."l_extendedprice" AS "l_extendedprice", F0."l_discount" AS "l_discount", F0."l_tax" AS "l_tax", F0."l_returnflag" AS "l_returnflag", F0."l_linestatus" AS "l_linestatus", F0."l_shipdate" AS "l_shipdate", F0."l_commitdate" AS "l_commitdate", F0."l_receiptdate" AS "l_receiptdate", F0."l_shipinstruct" AS "l_shipinstruct", F0."l_shipmode" AS "l_shipmode", F0."l_comment" AS "l_comment", set_bit(0::bit(64), binary_search_array_pos(ARRAY [1,1588,3118,4711,6239,7939,9506,10945,12507,14223,15717,17272,18873,20420,22042,23629,25317,26921,28650,30250,31871,33340,34881,36283,37795,39337,40917,42592,44180,45759,47338,48829,50439,51987,53541,54983,56510,58123,59683,61312,62770,64291,65789,67425,68893,70521,72098,73525,75022,76599,78164,79695,81181,82740,84263,85951,87527,89096,90688,92337,93915,95362,96929,98420,100001], l_suppkey) - 1,1 )    AS "PROV_lineitem1"
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
SELECT F0."l_orderkey" AS "l_orderkey", F0."l_partkey" AS "l_partkey", F0."l_suppkey" AS "l_suppkey", F0."l_linenumber" AS "l_linenumber", F0."l_quantity" AS "l_quantity", F0."l_extendedprice" AS "l_extendedprice", F0."l_discount" AS "l_discount", F0."l_tax" AS "l_tax", F0."l_returnflag" AS "l_returnflag", F0."l_linestatus" AS "l_linestatus", F0."l_shipdate" AS "l_shipdate", F0."l_commitdate" AS "l_commitdate", F0."l_receiptdate" AS "l_receiptdate", F0."l_shipinstruct" AS "l_shipinstruct", F0."l_shipmode" AS "l_shipmode", F0."l_comment" AS "l_comment", set_bit(0::bit(64), binary_search_array_pos(ARRAY [1,1588,3118,4711,6239,7939,9506,10945,12507,14223,15717,17272,18873,20420,22042,23629,25317,26921,28650,30250,31871,33340,34881,36283,37795,39337,40917,42592,44180,45759,47338,48829,50439,51987,53541,54983,56510,58123,59683,61312,62770,64291,65789,67425,68893,70521,72098,73525,75022,76599,78164,79695,81181,82740,84263,85951,87527,89096,90688,92337,93915,95362,96929,98420,100001], l_suppkey) - 1,1 )   AS "PROV_lineitem2"
FROM "lineitem" AS F0) F0
WHERE ((F0."l_shipdate" >= '1996-12-01') and (F0."l_shipdate" < '1997-03-01'))
GROUP BY F0."l_suppkey") F0
) m1, 
(SELECT sum((F0."l_extendedprice" * (1 - F0."l_discount"))) AS "AGGR_0", F0."l_suppkey" AS "GROUP_0", fast_bit_or(F0."PROV_lineitem2"::bit(64)) AS "PROV_lineitem2"
FROM (
SELECT F0."l_orderkey" AS "l_orderkey", F0."l_partkey" AS "l_partkey", F0."l_suppkey" AS "l_suppkey", F0."l_linenumber" AS "l_linenumber", F0."l_quantity" AS "l_quantity", F0."l_extendedprice" AS "l_extendedprice", F0."l_discount" AS "l_discount", F0."l_tax" AS "l_tax", F0."l_returnflag" AS "l_returnflag", F0."l_linestatus" AS "l_linestatus", F0."l_shipdate" AS "l_shipdate", F0."l_commitdate" AS "l_commitdate", F0."l_receiptdate" AS "l_receiptdate", F0."l_shipinstruct" AS "l_shipinstruct", F0."l_shipmode" AS "l_shipmode", F0."l_comment" AS "l_comment", 
set_bit(0::bit(64), binary_search_array_pos(ARRAY [1,1588,3118,4711,6239,7939,9506,10945,12507,14223,15717,17272,18873,20420,22042,23629,25317,26921,28650,30250,31871,33340,34881,36283,37795,39337,40917,42592,44180,45759,47338,48829,50439,51987,53541,54983,56510,58123,59683,61312,62770,64291,65789,67425,68893,70521,72098,73525,75022,76599,78164,79695,81181,82740,84263,85951,87527,89096,90688,92337,93915,95362,96929,98420,100001], l_suppkey) - 1,1 )    AS "PROV_lineitem2"
FROM "lineitem" AS F0) F0
WHERE ((F0."l_shipdate" >= '1996-12-01') and (F0."l_shipdate" < '1997-03-01'))
GROUP BY F0."l_suppkey") m2
where m1."AGGR_0" = m2."AGGR_0"
)
F0) F1)) F0
WHERE (F0."total_revenue" = F0."total_revenue2")) F1)) F0
WHERE (F0."s_suppkey" = F0."supplier_no")) F0;
