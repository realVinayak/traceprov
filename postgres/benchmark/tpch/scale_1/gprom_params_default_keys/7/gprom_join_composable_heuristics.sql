
SELECT F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey", F0_0.supp_nation AS supp_nation, F0_0.cust_nation AS cust_nation, F0_0.l_year AS l_year
FROM (
SELECT F0_0."GROUP_0" AS supp_nation, F0_0."GROUP_1" AS cust_nation, F0_0."GROUP_2" AS l_year, F1_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F1_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey"
FROM ((
SELECT F4_0.n_name AS "GROUP_0", F5_0.n_name AS "GROUP_1", date_part('YEAR', (F1_0.l_shipdate)::date) AS "GROUP_2"
FROM ((((((
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey
FROM supplier F0_0) F0_0 CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_shipdate AS l_shipdate
FROM lineitem F0_0) F1_0) CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey
FROM orders F0_0) F2_0) CROSS JOIN (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey
FROM customer F0_0) F3_0) CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name
FROM nation F0_0) F4_0) CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name
FROM nation F0_0) F5_0)
WHERE ((((((((F0_0.s_suppkey = F1_0.l_suppkey) AND (F2_0.o_orderkey = F1_0.l_orderkey)) AND (F3_0.c_custkey = F2_0.o_custkey)) AND (F0_0.s_nationkey = F4_0.n_nationkey)) AND (F3_0.c_nationkey = F5_0.n_nationkey)) AND (((F4_0.n_name = 'FRANCE') AND (F5_0.n_name = 'GERMANY')) OR ((F4_0.n_name = 'GERMANY') AND (F5_0.n_name = 'FRANCE')))) AND (F1_0.l_shipdate >= '1995-01-01')) AND (F1_0.l_shipdate <= '1996-12-31'))
GROUP BY F4_0.n_name, F5_0.n_name, date_part('YEAR', (F1_0.l_shipdate)::date)) F0_0 JOIN (
SELECT F0_0.n_name AS "_P_SIDE_GROUP_0", F0_0."n_name1" AS "_P_SIDE_GROUP_1", date_part('YEAR', (F0_0.l_shipdate)::date) AS "_P_SIDE_GROUP_2", F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey"
FROM (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.l_orderkey AS l_orderkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_shipdate AS l_shipdate, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey, F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F1_0.n_nationkey AS "n_nationkey1", F1_0.n_name AS "n_name1", F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F1_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey"
FROM ((
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.l_orderkey AS l_orderkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_shipdate AS l_shipdate, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey, F1_0.n_nationkey AS n_nationkey, F1_0.n_name AS n_name, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM ((
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.l_orderkey AS l_orderkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_shipdate AS l_shipdate, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F1_0.c_custkey AS c_custkey, F1_0.c_nationkey AS c_nationkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_customer_c__custkey AS prov_customer_c__custkey
FROM ((
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.l_orderkey AS l_orderkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_shipdate AS l_shipdate, F1_0.o_orderkey AS o_orderkey, F1_0.o_custkey AS o_custkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F1_0.l_orderkey AS l_orderkey, F1_0.l_suppkey AS l_suppkey, F1_0.l_shipdate AS l_shipdate, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.s_suppkey AS prov_supplier_s__suppkey
FROM supplier F0_0) F0_0 CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_shipdate AS l_shipdate, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderkey AS prov_orders_o__orderkey
FROM orders F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey, F0_0.c_custkey AS prov_customer_c__custkey
FROM customer F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_nationkey AS prov_nation_n__nationkey
FROM nation F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_nationkey AS "prov_nation_1_n__nationkey"
FROM nation F0_0) F1_0)) F0_0
WHERE ((((((((F0_0.s_suppkey = F0_0.l_suppkey) AND (F0_0.o_orderkey = F0_0.l_orderkey)) AND (F0_0.c_custkey = F0_0.o_custkey)) AND (F0_0.s_nationkey = F0_0.n_nationkey)) AND (F0_0.c_nationkey = F0_0."n_nationkey1")) AND (((F0_0.n_name = 'FRANCE') AND (F0_0."n_name1" = 'GERMANY')) OR ((F0_0.n_name = 'GERMANY') AND (F0_0."n_name1" = 'FRANCE')))) AND (F0_0.l_shipdate >= '1995-01-01')) AND (F0_0.l_shipdate <= '1996-12-31'))) F1_0 ON (((F0_0."GROUP_2" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_2") AND ((F0_0."GROUP_1" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_1") AND (F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0")))))
ORDER BY supp_nation ASC NULLS LAST, cust_nation ASC NULLS LAST, l_year ASC NULLS LAST) F0_0;


