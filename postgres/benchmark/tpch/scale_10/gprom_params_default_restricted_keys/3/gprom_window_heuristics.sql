
SELECT F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.l_orderkey AS l_orderkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_shippriority AS o_shippriority
FROM (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_shippriority AS o_shippriority, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, dense_rank() OVER ( ORDER BY F0_0.revenue DESC NULLS LAST, F0_0.o_orderdate ASC NULLS LAST, F0_0._result_tid) AS _result_tid
FROM (
SELECT F0_0."AGG_GB_ARG1" AS l_orderkey, F0_0."AGGR_0" AS revenue, F0_0."AGG_GB_ARG2" AS o_orderdate, F0_0."AGG_GB_ARG3" AS o_shippriority, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, dense_rank() OVER ( ORDER BY F0_0."AGG_GB_ARG1", F0_0."AGG_GB_ARG2", F0_0."AGG_GB_ARG3") AS _result_tid
FROM (
SELECT (F0_0.l_extendedprice * (1 - F0_0.l_discount)) AS "AGG_GB_ARG0", F0_0.l_orderkey AS "AGG_GB_ARG1", F0_0.o_orderdate AS "AGG_GB_ARG2", F0_0.o_shippriority AS "AGG_GB_ARG3", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, sum((F0_0.l_extendedprice * (1 - F0_0.l_discount))) OVER (PARTITION BY F0_0.l_orderkey, F0_0.o_orderdate, F0_0.o_shippriority) AS "AGGR_0"
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_mktsegment AS c_mktsegment, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_shippriority AS o_shippriority, F1_0.l_orderkey AS l_orderkey, F1_0.l_extendedprice AS l_extendedprice, F1_0.l_discount AS l_discount, F1_0.l_shipdate AS l_shipdate, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_mktsegment AS c_mktsegment, F1_0.o_orderkey AS o_orderkey, F1_0.o_custkey AS o_custkey, F1_0.o_orderdate AS o_orderdate, F1_0.o_shippriority AS o_shippriority, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_mktsegment AS c_mktsegment, F0_0.c_custkey AS prov_customer_c__custkey
FROM customer F0_0) F0_0 CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_shippriority AS o_shippriority, F0_0.o_orderkey AS prov_orders_o__orderkey
FROM orders F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_shipdate AS l_shipdate, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F1_0)) F0_0
WHERE (((((F0_0.c_mktsegment = 'BUILDING') AND (F0_0.c_custkey = F0_0.o_custkey)) AND (F0_0.l_orderkey = F0_0.o_orderkey)) AND (F0_0.o_orderdate < '1995-03-15')) AND (F0_0.l_shipdate > '1995-03-15'))) F0_0
ORDER BY revenue DESC NULLS LAST, o_orderdate ASC NULLS LAST) F0_0) F0_0
WHERE (F0_0._result_tid <= 10);


