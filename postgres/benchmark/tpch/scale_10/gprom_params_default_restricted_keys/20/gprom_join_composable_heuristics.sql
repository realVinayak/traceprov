
SELECT F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.s_name AS s_name, F0_0.s_address AS s_address
FROM (
SELECT F0_0.s_name AS s_name, F0_0.s_address AS s_address, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.s_name AS s_name, F0_0.s_address AS s_address, F0_0.s_nationkey AS s_nationkey, F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F1_0."nesting_eval_3" AS "nesting_eval_3", F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F1_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F1_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F1_0.prov_part_p__partkey AS prov_part_p__partkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_name AS s_name, F0_0.s_address AS s_address, F0_0.s_nationkey AS s_nationkey, F1_0.n_nationkey AS n_nationkey, F1_0.n_name AS n_name, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM ((
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_name AS s_name, F0_0.s_address AS s_address, F0_0.s_nationkey AS s_nationkey, F0_0.s_suppkey AS prov_supplier_s__suppkey
FROM supplier F0_0) F0_0 CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_nationkey AS prov_nation_n__nationkey
FROM nation F0_0) F1_0)) F0_0, LATERAL (
SELECT (CASE  WHEN ((F0_1."nesting_eval_3") IS NULL) THEN FALSE WHEN (F0_1."nesting_eval_3" = 1) THEN (NULL)::bool ELSE (F0_1."nesting_eval_3" = 2) END) AS "nesting_eval_3", F1_1.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F1_1.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F1_1.prov_part_p__partkey AS prov_part_p__partkey, F1_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT max((CASE  WHEN (F0_0.s_suppkey = F0_1.ps_suppkey) THEN 2 WHEN (((F0_0.s_suppkey) IS NULL) OR ((F0_1.ps_suppkey) IS NULL)) THEN 1 ELSE 0 END)) AS "nesting_eval_3"
FROM (
SELECT F0_1.ps_partkey AS ps_partkey, F0_1.ps_suppkey AS ps_suppkey, F0_1.ps_availqty AS ps_availqty
FROM partsupp F0_1) F0_1, LATERAL (
SELECT (CASE  WHEN ((max((CASE  WHEN (F0_1.ps_partkey = F0_2.p_partkey) THEN 2 WHEN (((F0_1.ps_partkey) IS NULL) OR ((F0_2.p_partkey) IS NULL)) THEN 1 ELSE 0 END))) IS NULL) THEN FALSE WHEN (max((CASE  WHEN (F0_1.ps_partkey = F0_2.p_partkey) THEN 2 WHEN (((F0_1.ps_partkey) IS NULL) OR ((F0_2.p_partkey) IS NULL)) THEN 1 ELSE 0 END)) = 1) THEN (NULL)::bool ELSE (max((CASE  WHEN (F0_1.ps_partkey = F0_2.p_partkey) THEN 2 WHEN (((F0_1.ps_partkey) IS NULL) OR ((F0_2.p_partkey) IS NULL)) THEN 1 ELSE 0 END)) = 2) END) AS "nesting_eval_1"
FROM (
SELECT F0_2.p_partkey AS p_partkey, F0_2.p_name AS p_name
FROM part F0_2) F0_2
WHERE (F0_2.p_name LIKE 'forest%')) F1_1, LATERAL (
SELECT (0.500000 * sum(F0_2.l_quantity)) AS "nesting_eval_2"
FROM (
SELECT F0_2.l_partkey AS l_partkey, F0_2.l_suppkey AS l_suppkey, F0_2.l_quantity AS l_quantity, F0_2.l_shipdate AS l_shipdate
FROM lineitem F0_2) F0_2
WHERE ((((F0_2.l_partkey = F0_1.ps_partkey) AND (F0_2.l_suppkey = F0_1.ps_suppkey)) AND (F0_2.l_shipdate >= '1994-01-01')) AND (F0_2.l_shipdate < '1995-01-01'))) F2_1
WHERE (F1_1."nesting_eval_1" AND (F0_1.ps_availqty > F2_1."nesting_eval_2"))) F0_1 LEFT OUTER JOIN (
SELECT F0_1.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_1.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_1.prov_part_p__partkey AS prov_part_p__partkey, F0_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_1.ps_availqty AS ps_availqty, F0_1."nesting_eval_1" AS "nesting_eval_1", F1_1."nesting_eval_2" AS "nesting_eval_2", F0_1.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_1.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_1.prov_part_p__partkey AS prov_part_p__partkey, F1_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_1.ps_partkey AS ps_partkey, F0_1.ps_suppkey AS ps_suppkey, F0_1.ps_availqty AS ps_availqty, F1_1."nesting_eval_1" AS "nesting_eval_1", F0_1.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_1.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F1_1.prov_part_p__partkey AS prov_part_p__partkey
FROM (
SELECT F0_1.ps_partkey AS ps_partkey, F0_1.ps_suppkey AS ps_suppkey, F0_1.ps_availqty AS ps_availqty, F0_1.ps_partkey AS prov_partsupp_ps__partkey, F0_1.ps_suppkey AS prov_partsupp_ps__suppkey
FROM partsupp F0_1) F0_1, LATERAL (
SELECT (CASE  WHEN ((F0_2."nesting_eval_1") IS NULL) THEN FALSE WHEN (F0_2."nesting_eval_1" = 1) THEN (NULL)::bool ELSE (F0_2."nesting_eval_1" = 2) END) AS "nesting_eval_1", F1_2.prov_part_p__partkey AS prov_part_p__partkey
FROM ((
SELECT max((CASE  WHEN (F0_1.ps_partkey = F0_2.p_partkey) THEN 2 WHEN (((F0_1.ps_partkey) IS NULL) OR ((F0_2.p_partkey) IS NULL)) THEN 1 ELSE 0 END)) AS "nesting_eval_1"
FROM (
SELECT F0_2.p_partkey AS p_partkey, F0_2.p_name AS p_name
FROM part F0_2) F0_2
WHERE (F0_2.p_name LIKE 'forest%')) F0_2 LEFT OUTER JOIN (
SELECT F0_2.prov_part_p__partkey AS prov_part_p__partkey
FROM (
SELECT F0_2.p_name AS p_name, F0_2.p_partkey AS prov_part_p__partkey
FROM part F0_2) F0_2
WHERE (F0_2.p_name LIKE 'forest%')) F1_2 ON ((1 = 1)))) F1_1) F0_1, LATERAL (
SELECT (0.500000 * F0_2."AGGR_0") AS "nesting_eval_2", F1_2.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_2.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT sum(F0_2.l_quantity) AS "AGGR_0"
FROM (
SELECT F0_2.l_partkey AS l_partkey, F0_2.l_suppkey AS l_suppkey, F0_2.l_quantity AS l_quantity, F0_2.l_shipdate AS l_shipdate
FROM lineitem F0_2) F0_2
WHERE ((((F0_2.l_partkey = F0_1.ps_partkey) AND (F0_2.l_suppkey = F0_1.ps_suppkey)) AND (F0_2.l_shipdate >= '1994-01-01')) AND (F0_2.l_shipdate < '1995-01-01'))) F0_2 LEFT OUTER JOIN (
SELECT F0_2.l_partkey AS l_partkey, F0_2.l_suppkey AS l_suppkey, F0_2.l_shipdate AS l_shipdate, F0_2.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_2.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_2.l_partkey AS l_partkey, F0_2.l_suppkey AS l_suppkey, F0_2.l_shipdate AS l_shipdate, F0_2.l_orderkey AS prov_lineitem_l__orderkey, F0_2.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_2) F0_2
WHERE ((((F0_2.l_partkey = F0_1.ps_partkey) AND (F0_2.l_suppkey = F0_1.ps_suppkey)) AND (F0_2.l_shipdate >= '1994-01-01')) AND (F0_2.l_shipdate < '1995-01-01'))) F1_2 ON ((1 = 1)))) F1_1) F0_1
WHERE (F0_1."nesting_eval_1" AND (F0_1.ps_availqty > F0_1."nesting_eval_2"))) F1_1 ON ((1 = 1)))) F1_0) F0_0
WHERE ((F0_0."nesting_eval_3" AND (F0_0.s_nationkey = F0_0.n_nationkey)) AND (F0_0.n_name = 'CANADA'))
ORDER BY s_name ASC NULLS LAST) F0_0;


