WITH temp_view_2 AS (
SELECT /*+ materialize */ F0_0.l_orderkey AS "GROUP_0", F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, dense_rank() OVER ( ORDER BY F0_0.l_orderkey) AS _result_tid, row_number() OVER (PARTITION BY F0_0.l_orderkey ORDER BY F0_0.l_orderkey) AS _setprov_dup_count
FROM (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_linenumber AS l_linenumber, F0_0.l_quantity AS l_quantity, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_tax AS l_tax, F0_0.l_returnflag AS l_returnflag, F0_0.l_linestatus AS l_linestatus, F0_0.l_shipdate AS l_shipdate, F0_0.l_commitdate AS l_commitdate, F0_0.l_receiptdate AS l_receiptdate, F0_0.l_shipinstruct AS l_shipinstruct, F0_0.l_shipmode AS l_shipmode, F0_0.l_comment AS l_comment, F0_0.l_linenumber AS prov_lineitem_l__linenumber, F0_0.l_orderkey AS prov_lineitem_l__orderkey, _tid2int8(F0_0.ctid) AS _result_tid, 1 AS _setprov_dup_count
FROM lineitem F0_0) F0_0
WHERE (F0_0.l_commitdate < F0_0.l_receiptdate)),
temp_view_1 AS (
SELECT /*+ materialize */ F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderstatus AS o_orderstatus, F0_0.o_totalprice AS o_totalprice, F0_0.o_orderdate AS o_orderdate, F0_0.o_orderpriority AS o_orderpriority, F0_0.o_clerk AS o_clerk, F0_0.o_shippriority AS o_shippriority, F0_0.o_comment AS o_comment, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0._result_tid AS left__result_tid, F0_0._setprov_dup_count AS left__setprov_dup_count, F1_0.l_orderkey AS l_orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0._result_tid AS right__result_tid, F1_0._setprov_dup_count AS right__setprov_dup_count, _mergerowid(F0_0._result_tid, F1_0._result_tid) AS _result_tid, greatest(F0_0._setprov_dup_count, F1_0._setprov_dup_count) AS _setprov_dup_count
FROM ((
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderstatus AS o_orderstatus, F0_0.o_totalprice AS o_totalprice, F0_0.o_orderdate AS o_orderdate, F0_0.o_orderpriority AS o_orderpriority, F0_0.o_clerk AS o_clerk, F0_0.o_shippriority AS o_shippriority, F0_0.o_comment AS o_comment, F0_0.o_orderkey AS prov_orders_o__orderkey, (F0_0.o_orderkey)::int8 AS _result_tid, 1 AS _setprov_dup_count
FROM orders F0_0) F0_0 JOIN (
SELECT F0_0."GROUP_0" AS l_orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_2) F0_0) F1_0 ON ((F0_0.o_orderkey = F1_0.l_orderkey)))),
temp_view_0 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0", F0_0."AGG_GB_ARG1" AS "GROUP_0", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, dense_rank() OVER ( ORDER BY F0_0."AGG_GB_ARG1") AS _result_tid, row_number() OVER (PARTITION BY F0_0."AGG_GB_ARG1" ORDER BY F0_0."AGG_GB_ARG1") AS _setprov_dup_count
FROM (
SELECT 1 AS "AGG_GB_ARG0", F0_0.o_orderpriority AS "AGG_GB_ARG1", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count, count((CASE  WHEN (1 = F0_0._setprov_dup_count) THEN 1 ELSE (NULL)::int8 END)) OVER (PARTITION BY F0_0.o_orderpriority) AS "AGGR_0"
FROM (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderstatus AS o_orderstatus, F0_0.o_totalprice AS o_totalprice, F0_0.o_orderdate AS o_orderdate, F0_0.o_orderpriority AS o_orderpriority, F0_0.o_clerk AS o_clerk, F0_0.o_shippriority AS o_shippriority, F0_0.o_comment AS o_comment, F0_0.l_orderkey AS l_orderkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_1) F0_0) F0_0
WHERE ((F0_0.o_orderdate >= '1993-07-01') AND (F0_0.o_orderdate < '1993-10-01'))) F0_0)
SELECT F0_0.o_orderpriority AS o_orderpriority, F0_0.order_count AS order_count, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey
FROM (
SELECT F0_0."GROUP_0" AS o_orderpriority, F0_0."AGGR_0" AS order_count, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_0) F0_0
ORDER BY o_orderpriority ASC NULLS LAST) F0_0;


