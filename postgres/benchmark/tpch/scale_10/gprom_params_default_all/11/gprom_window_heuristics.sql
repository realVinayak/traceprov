
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.value AS value, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F0_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F0_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey"
FROM (
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.value AS value, F1_0."(sum((ps_supplycost*ps_availqty))*0000010)" AS "(sum((ps_supplycost*ps_availqty))*0000010)", F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F1_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F1_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F1_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F1_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey"
FROM ((
SELECT F0_0.ps_partkey AS ps_partkey, sum((F0_0.ps_supplycost * F0_0.ps_availqty)) OVER (PARTITION BY F0_0.ps_partkey) AS value, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM (
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey, F0_0.ps_availqty AS ps_availqty, F0_0.ps_supplycost AS ps_supplycost, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F1_0.n_nationkey AS n_nationkey, F1_0.n_name AS n_name, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey
FROM ((
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey, F0_0.ps_availqty AS ps_availqty, F0_0.ps_supplycost AS ps_supplycost, F1_0.s_suppkey AS s_suppkey, F1_0.s_nationkey AS s_nationkey, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F1_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey
FROM ((
SELECT F0_0.ps_partkey AS ps_partkey, F0_0.ps_suppkey AS ps_suppkey, F0_0.ps_availqty AS ps_availqty, F0_0.ps_supplycost AS ps_supplycost, F0_0.ps_partkey AS prov_partsupp_ps__partkey, F0_0.ps_suppkey AS prov_partsupp_ps__suppkey
FROM partsupp F0_0) F0_0 CROSS JOIN (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.s_suppkey AS prov_supplier_s__suppkey
FROM supplier F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_nationkey AS prov_nation_n__nationkey
FROM nation F0_0) F1_0)) F0_0
WHERE (((F0_0.ps_suppkey = F0_0.s_suppkey) AND (F0_0.s_nationkey = F0_0.n_nationkey)) AND (F0_0.n_name = 'GERMANY'))) F0_0 CROSS JOIN (
SELECT (F0_0."AGGR_0" * 0.000010) AS "(sum((ps_supplycost*ps_availqty))*0000010)", F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F0_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F0_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey"
FROM (
SELECT F0_0."AGG_GB_ARG0" AS "AGG_GB_ARG0", F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F0_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F0_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey", F0_0._result_tid AS _result_tid, sum(F0_0."AGG_GB_ARG0") OVER () AS "AGGR_0", count(1) OVER () AS __dummy_cnt
FROM ((
SELECT (F0_0.ps_supplycost * F0_0.ps_availqty) AS "AGG_GB_ARG0", F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F0_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F0_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey", F0_0._result_tid AS _result_tid
FROM (
SELECT F0_0.ps_suppkey AS ps_suppkey, F0_0.ps_availqty AS ps_availqty, F0_0.ps_supplycost AS ps_supplycost, F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F1_0.n_nationkey AS n_nationkey, F1_0.n_name AS n_name, F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F0_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", F1_0."prov_nation_1_n__nationkey" AS "prov_nation_1_n__nationkey", _mergerowid(F0_0._result_tid, F1_0._result_tid) AS _result_tid
FROM ((
SELECT F0_0.ps_suppkey AS ps_suppkey, F0_0.ps_availqty AS ps_availqty, F0_0.ps_supplycost AS ps_supplycost, F1_0.s_suppkey AS s_suppkey, F1_0.s_nationkey AS s_nationkey, F0_0."prov_partsupp_1_ps__partkey" AS "prov_partsupp_1_ps__partkey", F0_0."prov_partsupp_1_ps__suppkey" AS "prov_partsupp_1_ps__suppkey", F1_0."prov_supplier_1_s__suppkey" AS "prov_supplier_1_s__suppkey", _mergerowid(F0_0._result_tid, F1_0._result_tid) AS _result_tid
FROM ((
SELECT F0_0.ps_suppkey AS ps_suppkey, F0_0.ps_availqty AS ps_availqty, F0_0.ps_supplycost AS ps_supplycost, F0_0.ps_partkey AS "prov_partsupp_1_ps__partkey", F0_0.ps_suppkey AS "prov_partsupp_1_ps__suppkey", _tid2int8(F0_0.ctid) AS _result_tid
FROM partsupp F0_0) F0_0 CROSS JOIN (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_nationkey AS s_nationkey, F0_0.s_suppkey AS "prov_supplier_1_s__suppkey", (F0_0.s_suppkey)::int8 AS _result_tid
FROM supplier F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_nationkey AS "prov_nation_1_n__nationkey", (F0_0.n_nationkey)::int8 AS _result_tid
FROM nation F0_0) F1_0)) F0_0
WHERE (((F0_0.ps_suppkey = F0_0.s_suppkey) AND (F0_0.s_nationkey = F0_0.n_nationkey)) AND (F0_0.n_name = 'GERMANY')) UNION ALL (SELECT NULL AS "AGG_GB_ARG0", NULL AS "prov_partsupp_1_ps__partkey", NULL AS "prov_partsupp_1_ps__suppkey", NULL AS "prov_supplier_1_s__suppkey", NULL AS "prov_nation_1_n__nationkey", -1 AS _result_tid))) F0_0) F0_0
WHERE ((F0_0.__dummy_cnt = 1) OR (F0_0._result_tid <> -1))) F1_0)) F0_0
WHERE (F0_0.value > F0_0."(sum((ps_supplycost*ps_availqty))*0000010)")
ORDER BY value DESC NULLS LAST;


