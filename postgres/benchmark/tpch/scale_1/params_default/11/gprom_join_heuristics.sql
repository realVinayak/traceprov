
SELECT F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F0_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F0_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey"
FROM (
SELECT F0_0.value AS value, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F0_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F0_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey"
FROM (
SELECT F0_0.value AS value, F1_0."(sum((ps_supplycost*ps_availqty))*0000100)" AS "(sum((ps_supplycost*ps_availqty))*0000100)", F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F1_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F1_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F1_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F1_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey"
FROM ((
SELECT F0_0."AGGR_0" AS value, F1_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F1_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F1_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM ((
SELECT sum((F0_0.ps_supplycost * F0_0.ps_availqty)) AS "AGGR_0", F0_0.ps_partkey AS "GROUP_0"
FROM (((
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey, F0_0.ps_availqty AS ps_availqty, F0_0.ps_supplycost AS ps_supplycost
FROM partsupp F0_0) F0_0 CROSS JOIN (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey
FROM supplier F0_0) F1_0) CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name
FROM nation F0_0) F2_0)
WHERE (((F0_0.ps_suppkey = F1_0.s_suppkey) AND (F1_0.s_nationkey = F2_0.n_nationkey)) AND (F2_0.n_name = 'GERMANY'))
GROUP BY F0_0.ps_partkey) F0_0 JOIN (
SELECT F0_0.ps_partkey AS "_P_SIDE_GROUP_0", F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM (
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F1_0.n_nationkey AS n_nationkey, F1_0.n_name AS n_name, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM ((
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey, F1_0.s_suppkey AS s_suppkey, F1_0.s_nationkey AS s_nationkey, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F1_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey
FROM ((
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey, F0_0.ps_partkey AS prov_partsupp_ps__partkey, F0_0.ps_suppkey AS prov_partsupp_ps__suppkey
FROM partsupp F0_0) F0_0 CROSS JOIN (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.s_suppkey AS prov_supplier_s__suppkey
FROM supplier F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_nationkey AS prov_nation_n__nationkey
FROM nation F0_0) F1_0)) F0_0
WHERE (((F0_0.ps_suppkey = F0_0.s_suppkey) AND (F0_0.s_nationkey = F0_0.n_nationkey)) AND (F0_0.n_name = 'GERMANY'))) F1_0 ON ((F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")))) F0_0 CROSS JOIN (
SELECT (F0_0."AGGR_0" * 0.000100) AS "(sum((ps_supplycost*ps_availqty))*0000100)", F1_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F1_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F1_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F1_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey"
FROM ((
SELECT sum((F0_0.ps_supplycost * F0_0.ps_availqty)) AS "AGGR_0"
FROM (((
SELECT F0_0.ps_suppkey AS ps_suppkey, F0_0.ps_availqty AS ps_availqty, F0_0.ps_supplycost AS ps_supplycost
FROM partsupp F0_0) F0_0 CROSS JOIN (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey
FROM supplier F0_0) F1_0) CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name
FROM nation F0_0) F2_0)
WHERE (((F0_0.ps_suppkey = F1_0.s_suppkey) AND (F1_0.s_nationkey = F2_0.n_nationkey)) AND (F2_0.n_name = 'GERMANY'))) F0_0 LEFT OUTER JOIN (
SELECT F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F0_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F0_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey"
FROM (
SELECT F0_0.ps_suppkey AS ps_suppkey, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F1_0.n_nationkey AS n_nationkey, F1_0.n_name AS n_name, F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F0_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F1_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey"
FROM ((
SELECT F0_0.ps_suppkey AS ps_suppkey, F1_0.s_suppkey AS s_suppkey, F1_0.s_nationkey AS s_nationkey, F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F1_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey"
FROM ((
SELECT F0_0.ps_suppkey AS ps_suppkey, F0_0.ps_partkey AS "prov_partsupp_1_ps__partkey", F0_0.ps_suppkey AS "prov_partsupp_1_ps__suppkey"
FROM partsupp F0_0) F0_0 CROSS JOIN (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.s_suppkey AS "prov_supplier_1_s__suppkey"
FROM supplier F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_nationkey AS "prov_nation_1_n__nationkey"
FROM nation F0_0) F1_0)) F0_0
WHERE (((F0_0.ps_suppkey = F0_0.s_suppkey) AND (F0_0.s_nationkey = F0_0.n_nationkey)) AND (F0_0.n_name = 'GERMANY'))) F1_0 ON ((1 = 1)))) F1_0)) F0_0
WHERE (F0_0.value > F0_0."(sum((ps_supplycost*ps_availqty))*0000100)")
ORDER BY value DESC NULLS LAST) F0_0;


