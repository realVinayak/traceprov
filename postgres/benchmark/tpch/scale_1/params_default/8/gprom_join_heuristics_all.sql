
SELECT F0_0."GROUP_0" AS o_year, (F0_0."AGGR_0" / F0_0."AGGR_1") AS mkt_share, F1_0.prov_part_p__partkey AS prov_part_p__partkey, F1_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F1_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey", F1_0.prov_region_r__regionkey AS prov_region_r__regionkey
FROM ((
SELECT sum((CASE  WHEN (F6_0.n_name = 'BRAZIL') THEN (F2_0.l_extendedprice * (1 - F2_0.l_discount)) ELSE 0 END)) AS "AGGR_0", sum((F2_0.l_extendedprice * (1 - F2_0.l_discount))) AS "AGGR_1", date_part('YEAR', (F3_0.o_orderdate)::date) AS "GROUP_0"
FROM ((((((((
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_type AS p_type
FROM part F0_0) F0_0 CROSS JOIN (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey
FROM supplier F0_0) F1_0) CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount
FROM lineitem F0_0) F2_0) CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate
FROM orders F0_0) F3_0) CROSS JOIN (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey
FROM customer F0_0) F4_0) CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_regionkey AS n_regionkey
FROM nation F0_0) F5_0) CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name
FROM nation F0_0) F6_0) CROSS JOIN (
SELECT F0_0.r_regionkey AS r_regionkey, F0_0.r_name AS r_name
FROM region F0_0) F7_0)
WHERE (((((((((((F0_0.p_partkey = F2_0.l_partkey) AND (F1_0.s_suppkey = F2_0.l_suppkey)) AND (F2_0.l_orderkey = F3_0.o_orderkey)) AND (F3_0.o_custkey = F4_0.c_custkey)) AND (F4_0.c_nationkey = F5_0.n_nationkey)) AND (F5_0.n_regionkey = F7_0.r_regionkey)) AND (F7_0.r_name = 'AMERICA')) AND (F1_0.s_nationkey = F6_0.n_nationkey)) AND (F3_0.o_orderdate >= '1995-01-01')) AND (F3_0.o_orderdate <= '1996-12-31')) AND (F0_0.p_type = 'ECONOMY ANODIZED STEEL'))
GROUP BY date_part('YEAR', (F3_0.o_orderdate)::date)) F0_0 JOIN (
SELECT date_part('YEAR', (F0_0.o_orderdate)::date) AS "_P_SIDE_GROUP_0", F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey", F0_0.prov_region_r__regionkey AS prov_region_r__regionkey
FROM (
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_type AS p_type, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey, F0_0.n_nationkey AS n_nationkey, F0_0.n_regionkey AS n_regionkey, F0_0."n_nationkey1" AS "n_nationkey1", F1_0.r_regionkey AS r_regionkey, F1_0.r_name AS r_name, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey", F1_0.prov_region_r__regionkey AS prov_region_r__regionkey
FROM ((
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_type AS p_type, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey, F0_0.n_nationkey AS n_nationkey, F0_0.n_regionkey AS n_regionkey, F1_0.n_nationkey AS "n_nationkey1", F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F1_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey"
FROM ((
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_type AS p_type, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey, F1_0.n_nationkey AS n_nationkey, F1_0.n_regionkey AS n_regionkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM ((
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_type AS p_type, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F1_0.c_custkey AS c_custkey, F1_0.c_nationkey AS c_nationkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_customer_c__custkey AS prov_customer_c__custkey
FROM ((
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_type AS p_type, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F1_0.o_orderkey AS o_orderkey, F1_0.o_custkey AS o_custkey, F1_0.o_orderdate AS o_orderdate, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_type AS p_type, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F1_0.l_orderkey AS l_orderkey, F1_0.l_partkey AS l_partkey, F1_0.l_suppkey AS l_suppkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_type AS p_type, F1_0.s_suppkey AS s_suppkey, F1_0.s_nationkey AS s_nationkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F1_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey
FROM ((
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_type AS p_type, F0_0.p_partkey AS prov_part_p__partkey
FROM part F0_0) F0_0 CROSS JOIN (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.s_suppkey AS prov_supplier_s__suppkey
FROM supplier F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_orderkey AS prov_orders_o__orderkey
FROM orders F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_nationkey AS c_nationkey, F0_0.c_custkey AS prov_customer_c__custkey
FROM customer F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_regionkey AS n_regionkey, F0_0.n_nationkey AS prov_nation_n__nationkey
FROM nation F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_nationkey AS "prov_nation_1_n__nationkey"
FROM nation F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.r_regionkey AS r_regionkey, F0_0.r_name AS r_name, F0_0.r_regionkey AS prov_region_r__regionkey
FROM region F0_0) F1_0)) F0_0
WHERE (((((((((((F0_0.p_partkey = F0_0.l_partkey) AND (F0_0.s_suppkey = F0_0.l_suppkey)) AND (F0_0.l_orderkey = F0_0.o_orderkey)) AND (F0_0.o_custkey = F0_0.c_custkey)) AND (F0_0.c_nationkey = F0_0.n_nationkey)) AND (F0_0.n_regionkey = F0_0.r_regionkey)) AND (F0_0.r_name = 'AMERICA')) AND (F0_0.s_nationkey = F0_0."n_nationkey1")) AND (F0_0.o_orderdate >= '1995-01-01')) AND (F0_0.o_orderdate <= '1996-12-31')) AND (F0_0.p_type = 'ECONOMY ANODIZED STEEL'))) F1_0 ON ((F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")))
ORDER BY o_year ASC NULLS LAST;


