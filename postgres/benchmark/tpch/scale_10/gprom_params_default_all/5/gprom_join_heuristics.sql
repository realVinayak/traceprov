
SELECT F0_0."GROUP_0" AS n_name, F0_0."AGGR_0" AS revenue, F1_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F1_0.prov_region_r__regionkey AS prov_region_r__regionkey
FROM ((
SELECT sum((F2_0.l_extendedprice * (1 - F2_0.l_discount))) AS "AGGR_0", F4_0.n_name AS "GROUP_0"
FROM ((((((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey
FROM customer F0_0) F0_0 CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate
FROM orders F0_0) F1_0) CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount
FROM lineitem F0_0) F2_0) CROSS JOIN (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey
FROM supplier F0_0) F3_0) CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_regionkey AS n_regionkey
FROM nation F0_0) F4_0) CROSS JOIN (
SELECT F0_0.r_regionkey AS r_regionkey, F0_0.r_name AS r_name
FROM region F0_0) F5_0)
WHERE (((((((((F0_0.c_custkey = F1_0.o_custkey) AND (F2_0.l_orderkey = F1_0.o_orderkey)) AND (F2_0.l_suppkey = F3_0.s_suppkey)) AND (F0_0.c_nationkey = F3_0.s_nationkey)) AND (F3_0.s_nationkey = F4_0.n_nationkey)) AND (F4_0.n_regionkey = F5_0.r_regionkey)) AND (F5_0.r_name = 'ASIA')) AND (F1_0.o_orderdate >= '1994-01-01')) AND (F1_0.o_orderdate < '1995-01-01'))
GROUP BY F4_0.n_name) F0_0 JOIN (
SELECT F0_0.n_name AS "_P_SIDE_GROUP_0", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0.prov_region_r__regionkey AS prov_region_r__regionkey
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.l_orderkey AS l_orderkey, F0_0.l_suppkey AS l_suppkey, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_regionkey AS n_regionkey, F1_0.r_regionkey AS r_regionkey, F1_0.r_name AS r_name, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F1_0.prov_region_r__regionkey AS prov_region_r__regionkey
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.l_orderkey AS l_orderkey, F0_0.l_suppkey AS l_suppkey, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F1_0.n_nationkey AS n_nationkey, F1_0.n_name AS n_name, F1_0.n_regionkey AS n_regionkey, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.l_orderkey AS l_orderkey, F0_0.l_suppkey AS l_suppkey, F1_0.s_suppkey AS s_suppkey, F1_0.s_nationkey AS s_nationkey, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F1_0.l_orderkey AS l_orderkey, F1_0.l_suppkey AS l_suppkey, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey, F1_0.o_orderkey AS o_orderkey, F1_0.o_custkey AS o_custkey, F1_0.o_orderdate AS o_orderdate, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey, F0_0.c_custkey AS prov_customer_c__custkey
FROM customer F0_0) F0_0 CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_orderkey AS prov_orders_o__orderkey
FROM orders F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber, F0_0.l_orderkey AS prov_lineitem_l__orderkey
FROM lineitem F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.s_suppkey AS prov_supplier_s__suppkey
FROM supplier F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_regionkey AS n_regionkey, F0_0.n_nationkey AS prov_nation_n__nationkey
FROM nation F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.r_regionkey AS r_regionkey, F0_0.r_name AS r_name, F0_0.r_regionkey AS prov_region_r__regionkey
FROM region F0_0) F1_0)) F0_0
WHERE (((((((((F0_0.c_custkey = F0_0.o_custkey) AND (F0_0.l_orderkey = F0_0.o_orderkey)) AND (F0_0.l_suppkey = F0_0.s_suppkey)) AND (F0_0.c_nationkey = F0_0.s_nationkey)) AND (F0_0.s_nationkey = F0_0.n_nationkey)) AND (F0_0.n_regionkey = F0_0.r_regionkey)) AND (F0_0.r_name = 'ASIA')) AND (F0_0.o_orderdate >= '1994-01-01')) AND (F0_0.o_orderdate < '1995-01-01'))) F1_0 ON ((F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")))
ORDER BY revenue DESC NULLS LAST;


