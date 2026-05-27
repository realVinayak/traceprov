
SELECT F0_0.c_name AS c_name, F0_0.c_custkey AS c_custkey, F0_0.o_orderkey AS o_orderkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_totalprice AS o_totalprice, F0_0."sum(l_quantity)" AS "sum(l_quantity)", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM (
SELECT F0_0.c_name AS c_name, F0_0.c_custkey AS c_custkey, F0_0.o_orderkey AS o_orderkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_totalprice AS o_totalprice, F0_0."sum(l_quantity)" AS "sum(l_quantity)", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber", dense_rank() OVER ( ORDER BY F0_0.o_totalprice DESC NULLS LAST, F0_0.o_orderdate ASC NULLS LAST, F0_0._result_tid) AS _result_tid
FROM (
SELECT F0_0."GROUP_0" AS c_name, F0_0."GROUP_1" AS c_custkey, F0_0."GROUP_2" AS o_orderkey, F0_0."GROUP_3" AS o_orderdate, F0_0."GROUP_4" AS o_totalprice, F0_0."AGGR_0" AS "sum(l_quantity)", F1_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F1_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber", dense_rank() OVER ( ORDER BY F0_0."GROUP_0", F0_0."GROUP_1", F0_0."GROUP_2", F0_0."GROUP_3", F0_0."GROUP_4") AS _result_tid
FROM ((
SELECT sum(F1_0.l_quantity) AS "AGGR_0", F0_0.c_name AS "GROUP_0", F0_0.c_custkey AS "GROUP_1", F2_0.o_orderkey AS "GROUP_2", F2_0.o_orderdate AS "GROUP_3", F2_0.o_totalprice AS "GROUP_4"
FROM (((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name
FROM customer F0_0) F0_0 CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_quantity AS l_quantity
FROM lineitem F0_0) F1_0) CROSS JOIN ((
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_totalprice AS o_totalprice, F0_0.o_orderdate AS o_orderdate
FROM orders F0_0) F2_0 JOIN (
SELECT F0_0.l_orderkey AS inner_l_orderkey
FROM lineitem F0_0
GROUP BY F0_0.l_orderkey
HAVING (sum(F0_0.l_quantity) > 300)) F3_0 ON ((F3_0.inner_l_orderkey = F0_0.c_custkey))))
WHERE ((F0_0.c_custkey = F2_0.o_custkey) AND (F2_0.o_orderkey = F1_0.l_orderkey))
GROUP BY F0_0.c_name, F0_0.c_custkey, F2_0.o_orderkey, F2_0.o_orderdate, F2_0.o_totalprice) F0_0 JOIN (
SELECT F0_0.c_name AS "_P_SIDE_GROUP_0", F0_0.c_custkey AS "_P_SIDE_GROUP_1", F0_0.o_orderkey AS "_P_SIDE_GROUP_2", F0_0.o_orderdate AS "_P_SIDE_GROUP_3", F0_0.o_totalprice AS "_P_SIDE_GROUP_4", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.l_orderkey AS l_orderkey, F1_0.o_orderkey AS o_orderkey, F1_0.o_custkey AS o_custkey, F1_0.o_totalprice AS o_totalprice, F1_0.o_orderdate AS o_orderdate, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F1_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F1_0.l_orderkey AS l_orderkey, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_custkey AS prov_customer_c__custkey
FROM customer F0_0) F0_0 CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_totalprice AS o_totalprice, F0_0.o_orderdate AS o_orderdate, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F1_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM ((
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_totalprice AS o_totalprice, F0_0.o_orderdate AS o_orderdate, F0_0.o_orderkey AS prov_orders_o__orderkey
FROM orders F0_0) F0_0 JOIN (
SELECT F0_0."GROUP_0" AS inner_l_orderkey, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM (
SELECT F0_0."AGGR_0" AS "AGGR_0", F0_0."GROUP_0" AS "GROUP_0", F1_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F1_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM ((
SELECT sum(F0_0.l_quantity) AS "AGGR_0", F0_0.l_orderkey AS "GROUP_0"
FROM lineitem F0_0
GROUP BY F0_0.l_orderkey) F0_0 JOIN (
SELECT F0_0.l_orderkey AS "_P_SIDE_GROUP_0", F0_0.l_orderkey AS "prov_lineitem_1_l__orderkey", F0_0.l_linenumber AS "prov_lineitem_1_l__linenumber"
FROM lineitem F0_0) F1_0 ON ((F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0")))) F0_0
WHERE (F0_0."AGGR_0" > 300)) F1_0 ON ((F1_0.inner_l_orderkey = F0_0.o_orderkey)))) F1_0)) F0_0
WHERE ((F0_0.c_custkey = F0_0.o_custkey) AND (F0_0.o_orderkey = F0_0.l_orderkey))) F1_0 ON (((F0_0."GROUP_4" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_4") AND ((F0_0."GROUP_3" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_3") AND ((F0_0."GROUP_2" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_2") AND ((F0_0."GROUP_1" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_1") AND (F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0")))))))
ORDER BY o_totalprice DESC NULLS LAST, o_orderdate ASC NULLS LAST) F0_0) F0_0
WHERE (F0_0._result_tid <= 100);


