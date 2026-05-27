
SELECT F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM (
SELECT F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, dense_rank() OVER ( ORDER BY F0_0.revenue DESC NULLS LAST, F0_0._result_tid) AS _result_tid
FROM (
SELECT F0_0."AGGR_0" AS revenue, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, dense_rank() OVER ( ORDER BY F0_0."AGG_GB_ARG1", F0_0."AGG_GB_ARG2", F0_0."AGG_GB_ARG3", F0_0."AGG_GB_ARG4", F0_0."AGG_GB_ARG5", F0_0."AGG_GB_ARG6", F0_0."AGG_GB_ARG7") AS _result_tid
FROM (
SELECT (F0_0.l_extendedprice * (1 - F0_0.l_discount)) AS "AGG_GB_ARG0", F0_0.c_custkey AS "AGG_GB_ARG1", F0_0.c_name AS "AGG_GB_ARG2", F0_0.c_acctbal AS "AGG_GB_ARG3", F0_0.c_phone AS "AGG_GB_ARG4", F0_0.n_name AS "AGG_GB_ARG5", F0_0.c_address AS "AGG_GB_ARG6", F0_0.c_comment AS "AGG_GB_ARG7", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, sum((F0_0.l_extendedprice * (1 - F0_0.l_discount))) OVER (PARTITION BY F0_0.c_custkey, F0_0.c_name, F0_0.c_acctbal, F0_0.c_phone, F0_0.n_name, F0_0.c_address, F0_0.c_comment) AS "AGGR_0"
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_comment AS c_comment, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.l_orderkey AS l_orderkey, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_returnflag AS l_returnflag, F1_0.n_nationkey AS n_nationkey, F1_0.n_name AS n_name, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_comment AS c_comment, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F1_0.l_orderkey AS l_orderkey, F1_0.l_extendedprice AS l_extendedprice, F1_0.l_discount AS l_discount, F1_0.l_returnflag AS l_returnflag, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_comment AS c_comment, F1_0.o_orderkey AS o_orderkey, F1_0.o_custkey AS o_custkey, F1_0.o_orderdate AS o_orderdate, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_comment AS c_comment, F0_0.c_custkey AS prov_customer_c__custkey
FROM customer F0_0) F0_0 CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_orderkey AS prov_orders_o__orderkey
FROM orders F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_returnflag AS l_returnflag, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_nationkey AS prov_nation_n__nationkey
FROM nation F0_0) F1_0)) F0_0
WHERE ((((((F0_0.c_custkey = F0_0.o_custkey) AND (F0_0.l_orderkey = F0_0.o_orderkey)) AND (F0_0.o_orderdate >= '1993-10-01')) AND (F0_0.o_orderdate < '1994-01-01')) AND (F0_0.l_returnflag = 'R')) AND (F0_0.c_nationkey = F0_0.n_nationkey))) F0_0
ORDER BY revenue DESC NULLS LAST) F0_0) F0_0
WHERE (F0_0._result_tid <= 20);


