
SELECT F0_0.s_name AS s_name, F0_0.s_address AS s_address, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_name AS s_name, F0_0.s_address AS s_address, F0_0.s_nationkey AS s_nationkey, F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F1_0.ps_suppkey AS ps_suppkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F1_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F1_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F1_0.prov_part_p__partkey AS prov_part_p__partkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_name AS s_name, F0_0.s_address AS s_address, F0_0.s_nationkey AS s_nationkey, F1_0.n_nationkey AS n_nationkey, F1_0.n_name AS n_name, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM ((
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_name AS s_name, F0_0.s_address AS s_address, F0_0.s_nationkey AS s_nationkey, F0_0.s_suppkey AS prov_supplier_s__suppkey
FROM supplier F0_0) F0_0 CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_nationkey AS prov_nation_n__nationkey
FROM nation F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0."GROUP_0" AS ps_suppkey, F1_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F1_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F1_0.prov_part_p__partkey AS prov_part_p__partkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F0_0.ps_suppkey AS "GROUP_0"
FROM (((
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey, F0_0.ps_availqty AS ps_availqty
FROM partsupp F0_0) F0_0 JOIN (
SELECT F0_0.p_partkey AS p_partkey
FROM (
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_name AS p_name
FROM part F0_0) F0_0
WHERE (F0_0.p_name LIKE 'forest%')) F1_0 ON ((F1_0.p_partkey = F0_0.ps_partkey))) JOIN (
SELECT (0.500000 * sum(F0_0.l_quantity)) AS computed, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey
FROM (
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_quantity AS l_quantity, F0_0.l_shipdate AS l_shipdate
FROM lineitem F0_0) F0_0
WHERE ((F0_0.l_shipdate >= '1994-01-01') AND (F0_0.l_shipdate < '1995-01-01'))
GROUP BY F0_0.l_partkey, F0_0.l_suppkey) F2_0 ON ((((F0_0.ps_partkey = F2_0.l_partkey) AND (F0_0.ps_suppkey = F2_0.l_suppkey)) AND (F0_0.ps_availqty > F2_0.computed))))
GROUP BY F0_0.ps_suppkey) F0_0 JOIN (
SELECT F0_0.ps_suppkey AS "_P_SIDE_GROUP_0", F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey, F0_0.ps_availqty AS ps_availqty, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F1_0.prov_part_p__partkey AS prov_part_p__partkey
FROM ((
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey, F0_0.ps_availqty AS ps_availqty, F0_0.ps_partkey AS prov_partsupp_ps__partkey, F0_0.ps_suppkey AS prov_partsupp_ps__suppkey
FROM partsupp F0_0) F0_0 JOIN (
SELECT F0_0.p_partkey AS p_partkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey
FROM (
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_name AS p_name, F0_0.p_partkey AS prov_part_p__partkey
FROM part F0_0) F0_0
WHERE (F0_0.p_name LIKE 'forest%')) F1_0 ON ((F1_0.p_partkey = F0_0.ps_partkey)))) F0_0 JOIN (
SELECT (0.500000 * F0_0."AGGR_0") AS computed, F0_0."GROUP_0" AS l_partkey, F0_0."GROUP_1" AS l_suppkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT sum(F0_0.l_quantity) AS "AGGR_0", F0_0.l_partkey AS "GROUP_0", F0_0.l_suppkey AS "GROUP_1"
FROM (
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_quantity AS l_quantity, F0_0.l_shipdate AS l_shipdate
FROM lineitem F0_0) F0_0
WHERE ((F0_0.l_shipdate >= '1994-01-01') AND (F0_0.l_shipdate < '1995-01-01'))
GROUP BY F0_0.l_partkey, F0_0.l_suppkey) F0_0 JOIN (
SELECT F0_0.l_partkey AS "_P_SIDE_GROUP_0", F0_0.l_suppkey AS "_P_SIDE_GROUP_1", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_shipdate AS l_shipdate, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F0_0
WHERE ((F0_0.l_shipdate >= '1994-01-01') AND (F0_0.l_shipdate < '1995-01-01'))) F1_0 ON (((F0_0."GROUP_1" = F1_0."_P_SIDE_GROUP_1") AND (F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0"))))) F1_0 ON ((((F0_0.ps_partkey = F1_0.l_partkey) AND (F0_0.ps_suppkey = F1_0.l_suppkey)) AND (F0_0.ps_availqty > F1_0.computed))))) F1_0 ON ((F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")))) F1_0)) F0_0
WHERE (((F0_0.ps_suppkey = F0_0.s_suppkey) AND (F0_0.s_nationkey = F0_0.n_nationkey)) AND (F0_0.n_name = 'CANADA'))
ORDER BY s_name ASC NULLS LAST;


