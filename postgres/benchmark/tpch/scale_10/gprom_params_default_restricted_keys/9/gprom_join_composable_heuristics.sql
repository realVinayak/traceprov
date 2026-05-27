
SELECT F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0.nation AS nation, F0_0.o_year AS o_year
FROM (
SELECT F0_0."GROUP_0" AS nation, F0_0."GROUP_1" AS o_year, F1_0.prov_part_p__partkey AS prov_part_p__partkey, F1_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F1_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM ((
SELECT F5_0.n_name AS "GROUP_0", date_part('YEAR', (F4_0.o_orderdate)::date) AS "GROUP_1"
FROM ((((((
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_name AS p_name
FROM part F0_0) F0_0 CROSS JOIN (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey
FROM supplier F0_0) F1_0) CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey
FROM lineitem F0_0) F2_0) CROSS JOIN (
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey
FROM partsupp F0_0) F3_0) CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_orderdate AS o_orderdate
FROM orders F0_0) F4_0) CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name
FROM nation F0_0) F5_0)
WHERE (((((((F1_0.s_suppkey = F2_0.l_suppkey) AND (F3_0.ps_suppkey = F2_0.l_suppkey)) AND (F3_0.ps_partkey = F2_0.l_partkey)) AND (F0_0.p_partkey = F2_0.l_partkey)) AND (F4_0.o_orderkey = F2_0.l_orderkey)) AND (F1_0.s_nationkey = F5_0.n_nationkey)) AND (F0_0.p_name LIKE '%green%'))
GROUP BY F5_0.n_name, date_part('YEAR', (F4_0.o_orderdate)::date)) F0_0 JOIN (
SELECT F0_0.n_name AS "_P_SIDE_GROUP_0", date_part('YEAR', (F0_0.o_orderdate)::date) AS "_P_SIDE_GROUP_1", F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM (
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_name AS p_name, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey, F0_0.o_orderkey AS o_orderkey, F0_0.o_orderdate AS o_orderdate, F1_0.n_nationkey AS n_nationkey, F1_0.n_name AS n_name, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM ((
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_name AS p_name, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey, F1_0.o_orderkey AS o_orderkey, F1_0.o_orderdate AS o_orderdate, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_name AS p_name, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F1_0.ps_partkey AS ps_partkey, F1_0.ps_suppkey AS ps_suppkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F1_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey
FROM ((
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_name AS p_name, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F1_0.l_orderkey AS l_orderkey, F1_0.l_partkey AS l_partkey, F1_0.l_suppkey AS l_suppkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_name AS p_name, F1_0.s_suppkey AS s_suppkey, F1_0.s_nationkey AS s_nationkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F1_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey
FROM ((
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_name AS p_name, F0_0.p_partkey AS prov_part_p__partkey
FROM part F0_0) F0_0 CROSS JOIN (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.s_suppkey AS prov_supplier_s__suppkey
FROM supplier F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey, F0_0.ps_partkey AS prov_partsupp_ps__partkey, F0_0.ps_suppkey AS prov_partsupp_ps__suppkey
FROM partsupp F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_orderkey AS prov_orders_o__orderkey
FROM orders F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_nationkey AS prov_nation_n__nationkey
FROM nation F0_0) F1_0)) F0_0
WHERE (((((((F0_0.s_suppkey = F0_0.l_suppkey) AND (F0_0.ps_suppkey = F0_0.l_suppkey)) AND (F0_0.ps_partkey = F0_0.l_partkey)) AND (F0_0.p_partkey = F0_0.l_partkey)) AND (F0_0.o_orderkey = F0_0.l_orderkey)) AND (F0_0.s_nationkey = F0_0.n_nationkey)) AND (F0_0.p_name LIKE '%green%'))) F1_0 ON (((F0_0."GROUP_1" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_1") AND (F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0"))))
ORDER BY nation ASC NULLS LAST, o_year DESC NULLS LAST) F0_0;


