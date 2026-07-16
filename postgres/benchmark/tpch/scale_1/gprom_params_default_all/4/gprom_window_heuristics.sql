
SELECT F0_0.o_orderpriority AS o_orderpriority, count((CASE  WHEN (1 = F0_0._setprov_dup_count) THEN 1 ELSE (NULL)::int8 END)) OVER (PARTITION BY F0_0.o_orderpriority) AS order_count, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.o_orderdate AS o_orderdate, F0_0.o_orderpriority AS o_orderpriority, F1_0."nesting_eval_1" AS "nesting_eval_1", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, greatest(F0_0.left__setprov_dup_count, F1_0.right__setprov_dup_count) AS _setprov_dup_count
FROM (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_orderpriority AS o_orderpriority, F0_0.o_orderkey AS prov_orders_o__orderkey, 1 AS left__setprov_dup_count
FROM orders F0_0) F0_0, LATERAL (
SELECT (F0_1."aggr_0" > 0) AS "nesting_eval_1", F0_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, ROW_NUMBER() OVER () AS right__setprov_dup_count
FROM (
SELECT F0_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_1._result_tid AS _result_tid, count(1) OVER () AS "aggr_0"
FROM ((
SELECT F0_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_1._result_tid AS _result_tid
FROM (
SELECT F0_1.l_orderkey AS l_orderkey, F0_1.l_commitdate AS l_commitdate, F0_1.l_receiptdate AS l_receiptdate, F0_1.l_orderkey AS prov_lineitem_l__orderkey, F0_1.l_linenumber AS prov_lineitem_l__linenumber, _tid2int8(F0_1.ctid) AS _result_tid
FROM lineitem F0_1) F0_1
WHERE ((F0_1.l_orderkey = F0_0.o_orderkey) AND (F0_1.l_commitdate < F0_1.l_receiptdate)) UNION ALL (SELECT NULL AS prov_lineitem_l__orderkey, NULL AS prov_lineitem_l__linenumber, -1 AS _result_tid))) F0_1) F0_1
WHERE ((F0_1."aggr_0" = 0) OR (F0_1._result_tid <> -1))) F1_0) F0_0
WHERE (((F0_0.o_orderdate >= '1993-07-01') AND (F0_0.o_orderdate < '1993-10-01')) AND F0_0."nesting_eval_1")
ORDER BY o_orderpriority ASC NULLS LAST;


