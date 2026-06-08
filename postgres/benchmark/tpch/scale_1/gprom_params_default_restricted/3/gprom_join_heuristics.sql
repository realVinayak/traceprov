
SELECT F1_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F2_0.l_orderkey AS l_orderkey, sum((F2_0.l_extendedprice * (1 - F2_0.l_discount))) AS revenue, F1_0.o_orderdate AS o_orderdate, F1_0.o_shippriority AS o_shippriority
FROM (((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_mktsegment AS c_mktsegment
FROM customer F0_0) F0_0 CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_shippriority AS o_shippriority
FROM orders F0_0) F1_0) CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_shipdate AS l_shipdate
FROM lineitem F0_0) F2_0)
WHERE (((((F0_0.c_mktsegment = 'BUILDING') AND (F0_0.c_custkey = F1_0.o_custkey)) AND (F2_0.l_orderkey = F1_0.o_orderkey)) AND (F1_0.o_orderdate < '1995-03-15')) AND (F2_0.l_shipdate > '1995-03-15'))
GROUP BY F2_0.l_orderkey, F1_0.o_orderdate, F1_0.o_shippriority
ORDER BY revenue DESC NULLS LAST, o_orderdate ASC NULLS LAST
LIMIT 10) F0_0 JOIN (
SELECT F0_0."GROUP_0" AS l_orderkey, F0_0."AGGR_0" AS revenue, F0_0."GROUP_1" AS o_orderdate, F0_0."GROUP_2" AS o_shippriority, F1_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT sum((F2_0.l_extendedprice * (1 - F2_0.l_discount))) AS "AGGR_0", F2_0.l_orderkey AS "GROUP_0", F1_0.o_orderdate AS "GROUP_1", F1_0.o_shippriority AS "GROUP_2"
FROM (((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_mktsegment AS c_mktsegment
FROM customer F0_0) F0_0 CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_shippriority AS o_shippriority
FROM orders F0_0) F1_0) CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_shipdate AS l_shipdate
FROM lineitem F0_0) F2_0)
WHERE (((((F0_0.c_mktsegment = 'BUILDING') AND (F0_0.c_custkey = F1_0.o_custkey)) AND (F2_0.l_orderkey = F1_0.o_orderkey)) AND (F1_0.o_orderdate < '1995-03-15')) AND (F2_0.l_shipdate > '1995-03-15'))
GROUP BY F2_0.l_orderkey, F1_0.o_orderdate, F1_0.o_shippriority) F0_0 JOIN (
SELECT F0_0.l_orderkey AS "_P_SIDE_GROUP_0", F0_0.o_orderdate AS "_P_SIDE_GROUP_1", F0_0.o_shippriority AS "_P_SIDE_GROUP_2", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_mktsegment AS c_mktsegment, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_shippriority AS o_shippriority, F1_0.l_orderkey AS l_orderkey, F1_0.l_shipdate AS l_shipdate, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_mktsegment AS c_mktsegment, F1_0.o_orderkey AS o_orderkey, F1_0.o_custkey AS o_custkey, F1_0.o_orderdate AS o_orderdate, F1_0.o_shippriority AS o_shippriority, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_mktsegment AS c_mktsegment, F0_0.c_custkey AS prov_customer_c__custkey
FROM customer F0_0) F0_0 CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_shippriority AS o_shippriority, F0_0.o_orderkey AS prov_orders_o__orderkey
FROM orders F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_shipdate AS l_shipdate, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F1_0)) F0_0
WHERE (((((F0_0.c_mktsegment = 'BUILDING') AND (F0_0.c_custkey = F0_0.o_custkey)) AND (F0_0.l_orderkey = F0_0.o_orderkey)) AND (F0_0.o_orderdate < '1995-03-15')) AND (F0_0.l_shipdate > '1995-03-15'))) F1_0 ON (((F0_0."GROUP_2" = F1_0."_P_SIDE_GROUP_2") AND ((F0_0."GROUP_1" = F1_0."_P_SIDE_GROUP_1") AND (F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")))))
ORDER BY revenue DESC NULLS LAST, o_orderdate ASC NULLS LAST) F1_0 ON (((((F0_0.l_orderkey = F1_0.l_orderkey) AND (F0_0.revenue = F1_0.revenue)) AND (F0_0.o_orderdate = F1_0.o_orderdate)) AND (F0_0.o_shippriority = F1_0.o_shippriority))));


