
SELECT F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_acctbal AS c_acctbal, F0_0.n_name AS n_name, F0_0.c_address AS c_address, F0_0.c_phone AS c_phone, F0_0.c_comment AS c_comment
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_acctbal AS c_acctbal, F0_0.n_name AS n_name, F0_0.c_address AS c_address, F0_0.c_phone AS c_phone, F0_0.c_comment AS c_comment, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, dense_rank() OVER ( ORDER BY F0_0.revenue DESC NULLS LAST, F0_0._result_tid) AS _result_tid
FROM (
SELECT F0_0."GROUP_0" AS c_custkey, F0_0."GROUP_1" AS c_name, F0_0."AGGR_0" AS revenue, F0_0."GROUP_2" AS c_acctbal, F0_0."GROUP_4" AS n_name, F0_0."GROUP_5" AS c_address, F0_0."GROUP_3" AS c_phone, F0_0."GROUP_6" AS c_comment, F1_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, dense_rank() OVER ( ORDER BY F0_0."GROUP_0", F0_0."GROUP_1", F0_0."GROUP_2", F0_0."GROUP_3", F0_0."GROUP_4", F0_0."GROUP_5", F0_0."GROUP_6") AS _result_tid
FROM ((
SELECT sum((F2_0.l_extendedprice * (1 - F2_0.l_discount))) AS "AGGR_0", F0_0.c_custkey AS "GROUP_0", F0_0.c_name AS "GROUP_1", F0_0.c_acctbal AS "GROUP_2", F0_0.c_phone AS "GROUP_3", F3_0.n_name AS "GROUP_4", F0_0.c_address AS "GROUP_5", F0_0.c_comment AS "GROUP_6"
FROM ((((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_comment AS c_comment
FROM customer F0_0) F0_0 CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate
FROM orders F0_0) F1_0) CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_returnflag AS l_returnflag
FROM lineitem F0_0) F2_0) CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name
FROM nation F0_0) F3_0)
WHERE ((((((F0_0.c_custkey = F1_0.o_custkey) AND (F2_0.l_orderkey = F1_0.o_orderkey)) AND (F1_0.o_orderdate >= '1993-10-01')) AND (F1_0.o_orderdate < '1994-01-01')) AND (F2_0.l_returnflag = 'R')) AND (F0_0.c_nationkey = F3_0.n_nationkey))
GROUP BY F0_0.c_custkey, F0_0.c_name, F0_0.c_acctbal, F0_0.c_phone, F3_0.n_name, F0_0.c_address, F0_0.c_comment) F0_0 JOIN (
SELECT F0_0.c_custkey AS "_P_SIDE_GROUP_0", F0_0.c_name AS "_P_SIDE_GROUP_1", F0_0.c_acctbal AS "_P_SIDE_GROUP_2", F0_0.c_phone AS "_P_SIDE_GROUP_3", F0_0.n_name AS "_P_SIDE_GROUP_4", F0_0.c_address AS "_P_SIDE_GROUP_5", F0_0.c_comment AS "_P_SIDE_GROUP_6", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_comment AS c_comment, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.l_orderkey AS l_orderkey, F0_0.l_returnflag AS l_returnflag, F1_0.n_nationkey AS n_nationkey, F1_0.n_name AS n_name, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_comment AS c_comment, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F1_0.l_orderkey AS l_orderkey, F1_0.l_returnflag AS l_returnflag, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_comment AS c_comment, F1_0.o_orderkey AS o_orderkey, F1_0.o_custkey AS o_custkey, F1_0.o_orderdate AS o_orderdate, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_comment AS c_comment, F0_0.c_custkey AS prov_customer_c__custkey
FROM customer F0_0) F0_0 CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_orderkey AS prov_orders_o__orderkey
FROM orders F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_returnflag AS l_returnflag, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_nationkey AS prov_nation_n__nationkey
FROM nation F0_0) F1_0)) F0_0
WHERE ((((((F0_0.c_custkey = F0_0.o_custkey) AND (F0_0.l_orderkey = F0_0.o_orderkey)) AND (F0_0.o_orderdate >= '1993-10-01')) AND (F0_0.o_orderdate < '1994-01-01')) AND (F0_0.l_returnflag = 'R')) AND (F0_0.c_nationkey = F0_0.n_nationkey))) F1_0 ON (((F0_0."GROUP_6" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_6") AND ((F0_0."GROUP_5" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_5") AND ((F0_0."GROUP_4" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_4") AND ((F0_0."GROUP_3" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_3") AND ((F0_0."GROUP_2" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_2") AND ((F0_0."GROUP_1" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_1") AND (F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0")))))))))
ORDER BY revenue DESC NULLS LAST) F0_0) F0_0
WHERE (F0_0._result_tid <= 20);


