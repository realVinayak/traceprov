WITH temp_view_1 AS (
SELECT /*+ materialize */ 1 AS "AGG_GB_ARG0", F0_0.o_orderpriority AS "AGG_GB_ARG1", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderstatus AS o_orderstatus, F0_0.o_totalprice AS o_totalprice, F0_0.o_orderdate AS o_orderdate, F0_0.o_orderpriority AS o_orderpriority, F0_0.o_clerk AS o_clerk, F0_0.o_shippriority AS o_shippriority, F0_0.o_comment AS o_comment, F1_0."nesting_eval_1" AS "nesting_eval_1", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderstatus AS o_orderstatus, F0_0.o_totalprice AS o_totalprice, F0_0.o_orderdate AS o_orderdate, F0_0.o_orderpriority AS o_orderpriority, F0_0.o_clerk AS o_clerk, F0_0.o_shippriority AS o_shippriority, F0_0.o_comment AS o_comment, F0_0.o_orderkey AS prov_orders_o__orderkey
FROM orders F0_0) F0_0, LATERAL (
SELECT (F0_1."aggr_0" > 0) AS "nesting_eval_1", F0_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT /*+ materialize */ F0_1."aggr_0" AS "aggr_0", F1_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT count(1) AS "aggr_0"
FROM lineitem F0_1
WHERE ((F0_1.l_orderkey = F0_0.o_orderkey) AND (F0_1.l_commitdate < F0_1.l_receiptdate))) F0_1 LEFT OUTER JOIN (
SELECT F0_1.l_orderkey AS l_orderkey, F0_1.l_partkey AS l_partkey, F0_1.l_suppkey AS l_suppkey, F0_1.l_linenumber AS l_linenumber, F0_1.l_quantity AS l_quantity, F0_1.l_extendedprice AS l_extendedprice, F0_1.l_discount AS l_discount, F0_1.l_tax AS l_tax, F0_1.l_returnflag AS l_returnflag, F0_1.l_linestatus AS l_linestatus, F0_1.l_shipdate AS l_shipdate, F0_1.l_commitdate AS l_commitdate, F0_1.l_receiptdate AS l_receiptdate, F0_1.l_shipinstruct AS l_shipinstruct, F0_1.l_shipmode AS l_shipmode, F0_1.l_comment AS l_comment, F0_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_1.l_orderkey AS l_orderkey, F0_1.l_partkey AS l_partkey, F0_1.l_suppkey AS l_suppkey, F0_1.l_linenumber AS l_linenumber, F0_1.l_quantity AS l_quantity, F0_1.l_extendedprice AS l_extendedprice, F0_1.l_discount AS l_discount, F0_1.l_tax AS l_tax, F0_1.l_returnflag AS l_returnflag, F0_1.l_linestatus AS l_linestatus, F0_1.l_shipdate AS l_shipdate, F0_1.l_commitdate AS l_commitdate, F0_1.l_receiptdate AS l_receiptdate, F0_1.l_shipinstruct AS l_shipinstruct, F0_1.l_shipmode AS l_shipmode, F0_1.l_comment AS l_comment, F0_1.l_orderkey AS prov_lineitem_l__orderkey, F0_1.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_1) F0_1
WHERE ((F0_1.l_orderkey = F0_0.o_orderkey) AND (F0_1.l_commitdate < F0_1.l_receiptdate))) F1_1 ON ((1 = 1)))) F0_1) F1_0) F0_0
WHERE (((F0_0.o_orderdate >= '1993-07-01') AND (F0_0.o_orderdate < '1993-10-01')) AND F0_0."nesting_eval_1")),
temp_view_0 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0", F0_0."GROUP_0" AS "GROUP_0", F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT count(1) AS "AGGR_0", F0_0.o_orderpriority AS "GROUP_0"
FROM orders F0_0, LATERAL (
SELECT (count(1) > 0) AS "nesting_eval_1"
FROM lineitem F0_1
WHERE ((F0_1.l_orderkey = F0_0.o_orderkey) AND (F0_1.l_commitdate < F0_1.l_receiptdate))) F1_0
WHERE (((F0_0.o_orderdate >= '1993-07-01') AND (F0_0.o_orderdate < '1993-10-01')) AND F1_0."nesting_eval_1")
GROUP BY F0_0.o_orderpriority) F0_0 JOIN (
SELECT F0_0."AGG_GB_ARG1" AS "_P_SIDE_GROUP_0", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (SELECT * FROM temp_view_1) F0_0) F1_0 ON ((F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0"))))
SELECT F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.o_orderpriority AS o_orderpriority
FROM (
SELECT F0_0."GROUP_0" AS o_orderpriority, F0_0."AGGR_0" AS order_count, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (SELECT * FROM temp_view_0) F0_0
ORDER BY o_orderpriority ASC NULLS LAST) F0_0;


