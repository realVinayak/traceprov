
SELECT F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM (
SELECT F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber", dense_rank() OVER ( ORDER BY F0_0.o_totalprice DESC NULLS LAST, F0_0.o_orderdate ASC NULLS LAST, F0_0._result_tid) AS _result_tid
FROM (
SELECT F0_0.o_orderdate AS o_orderdate, F0_0.o_totalprice AS o_totalprice, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber", dense_rank() OVER ( ORDER BY F0_0.c_name, F0_0.c_custkey, F0_0.o_orderkey, F0_0.o_orderdate, F0_0.o_totalprice) AS _result_tid
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.l_orderkey AS l_orderkey, F0_0.l_quantity AS l_quantity, F1_0.o_orderkey AS o_orderkey, F1_0.o_custkey AS o_custkey, F1_0.o_totalprice AS o_totalprice, F1_0.o_orderdate AS o_orderdate, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F1_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber", greatest(F0_0._setprov_dup_count, F1_0._setprov_dup_count) AS _setprov_dup_count
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F1_0.l_orderkey AS l_orderkey, F1_0.l_quantity AS l_quantity, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, greatest(F0_0._setprov_dup_count, F1_0._setprov_dup_count) AS _setprov_dup_count
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_custkey AS prov_customer_c__custkey, 1 AS _setprov_dup_count
FROM customer F0_0) F0_0 CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_quantity AS l_quantity, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber, 1 AS _setprov_dup_count
FROM lineitem F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_totalprice AS o_totalprice, F0_0.o_orderdate AS o_orderdate, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F1_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber", greatest(F0_0._setprov_dup_count, F1_0._setprov_dup_count) AS _setprov_dup_count
FROM ((
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_totalprice AS o_totalprice, F0_0.o_orderdate AS o_orderdate, F0_0.o_orderkey AS prov_orders_o__orderkey, 1 AS _setprov_dup_count
FROM orders F0_0) F0_0 JOIN (
SELECT F0_0."GROUP_0" AS inner_l_orderkey, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber", F0_0._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT F0_0."AGGR_0" AS "AGGR_0", F0_0.l_orderkey AS "GROUP_0", F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber", row_number() OVER (PARTITION BY F0_0.l_orderkey ORDER BY F0_0.l_orderkey) AS _setprov_dup_count
FROM (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_quantity AS l_quantity, F0_0.l_orderkey AS "prov_lineitem_1_l__orderkey", F0_0.l_linenumber AS "prov_lineitem_1_l__linenumber", sum(F0_0.l_quantity) OVER (PARTITION BY F0_0.l_orderkey) AS "AGGR_0"
FROM lineitem F0_0) F0_0) F0_0
WHERE (F0_0."AGGR_0" > 300)) F1_0 ON ((F1_0.inner_l_orderkey = F0_0.o_orderkey)))) F1_0)) F0_0
WHERE ((F0_0.c_custkey = F0_0.o_custkey) AND (F0_0.o_orderkey = F0_0.l_orderkey))
ORDER BY o_totalprice DESC NULLS LAST, o_orderdate ASC NULLS LAST) F0_0) F0_0
WHERE (F0_0._result_tid <= 100);


