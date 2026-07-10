WITH temp_view_2 AS (
SELECT /*+ materialize */ F0_0.s_suppkey AS s_suppkey, F0_0.s_name AS s_name, F0_0.s_address AS s_address, F0_0.s_nationkey AS s_nationkey, F0_0.s_phone AS s_phone, F0_0.s_acctbal AS s_acctbal, F0_0.s_comment AS s_comment, F1_0.n_nationkey AS n_nationkey, F1_0.n_name AS n_name, F1_0.n_regionkey AS n_regionkey, F1_0.n_comment AS n_comment, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F1_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, _mergerowid(F0_0._result_tid, F1_0._result_tid) AS _result_tid, greatest(F0_0._setprov_dup_count, F1_0._setprov_dup_count) AS _setprov_dup_count
FROM ((
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_name AS s_name, F0_0.s_address AS s_address, F0_0.s_nationkey AS s_nationkey, F0_0.s_phone AS s_phone, F0_0.s_acctbal AS s_acctbal, F0_0.s_comment AS s_comment, F0_0.s_suppkey AS prov_supplier_s__suppkey, (F0_0.s_suppkey)::int8 AS _result_tid, 1 AS _setprov_dup_count
FROM supplier F0_0) F0_0 CROSS JOIN (
SELECT F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_regionkey AS n_regionkey, F0_0.n_comment AS n_comment, F0_0.n_nationkey AS prov_nation_n__nationkey, (F0_0.n_nationkey)::int8 AS _result_tid, 1 AS _setprov_dup_count
FROM nation F0_0) F1_0)),
temp_view_4 AS (
SELECT /*+ materialize */ F0_2.p_partkey AS p_partkey
FROM part F0_2
WHERE (F0_2.p_name LIKE 'forest%')),
temp_view_3 AS (
SELECT /*+ materialize */ F0_1.ps_suppkey AS ps_suppkey
FROM partsupp F0_1, LATERAL (
SELECT (CASE  WHEN ((max((CASE  WHEN (F0_1.ps_partkey = F0_2.p_partkey) THEN 2 WHEN (((F0_1.ps_partkey) IS NULL) OR ((F0_2.p_partkey) IS NULL)) THEN 1 ELSE 0 END))) IS NULL) THEN FALSE WHEN (max((CASE  WHEN (F0_1.ps_partkey = F0_2.p_partkey) THEN 2 WHEN (((F0_1.ps_partkey) IS NULL) OR ((F0_2.p_partkey) IS NULL)) THEN 1 ELSE 0 END)) = 1) THEN (NULL)::bool ELSE (max((CASE  WHEN (F0_1.ps_partkey = F0_2.p_partkey) THEN 2 WHEN (((F0_1.ps_partkey) IS NULL) OR ((F0_2.p_partkey) IS NULL)) THEN 1 ELSE 0 END)) = 2) END) AS "nesting_eval_1"
FROM (SELECT * FROM temp_view_4) F0_2) F1_1, LATERAL (
SELECT F0_2."(0500000*sum(l_quantity))" AS "nesting_eval_2"
FROM (
SELECT /*+ materialize */ (0.500000 * sum(F0_2.l_quantity)) AS "(0500000*sum(l_quantity))"
FROM lineitem F0_2
WHERE ((((F0_2.l_partkey = F0_1.ps_partkey) AND (F0_2.l_suppkey = F0_1.ps_suppkey)) AND (F0_2.l_shipdate >= '1994-01-01')) AND (F0_2.l_shipdate < '1995-01-01'))) F0_2) F2_1
WHERE (F1_1."nesting_eval_1" AND (F0_1.ps_availqty > F2_1."nesting_eval_2"))),
temp_view_9 AS (
SELECT /*+ materialize */ F0_1.ps_partkey AS ps_partkey, F0_1.ps_suppkey AS ps_suppkey, F0_1.ps_availqty AS ps_availqty, F0_1.ps_supplycost AS ps_supplycost, F0_1.ps_comment AS ps_comment, F0_1.ps_partkey AS prov_partsupp_ps__partkey, F0_1.ps_suppkey AS prov_partsupp_ps__suppkey, _tid2int8(F0_1.ctid) AS _result_tid, 1 AS _setprov_dup_count
FROM partsupp F0_1),
temp_view_10 AS (
SELECT /*+ materialize */ F0_2.p_partkey AS p_partkey
FROM part F0_2
WHERE (F0_2.p_name LIKE 'forest%')),
temp_view_11 AS (
SELECT /*+ materialize */ F0_2.p_partkey AS p_partkey, F0_2.prov_part_p__partkey AS prov_part_p__partkey, F0_2._result_tid AS _result_tid, F0_2._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT F0_2.p_partkey AS p_partkey, F0_2.p_name AS p_name, F0_2.p_mfgr AS p_mfgr, F0_2.p_brand AS p_brand, F0_2.p_type AS p_type, F0_2.p_size AS p_size, F0_2.p_container AS p_container, F0_2.p_retailprice AS p_retailprice, F0_2.p_comment AS p_comment, F0_2.p_partkey AS prov_part_p__partkey, (F0_2.p_partkey)::int8 AS _result_tid, 1 AS _setprov_dup_count
FROM part F0_2) F0_2
WHERE (F0_2.p_name LIKE 'forest%')),
temp_view_8 AS (
SELECT /*+ materialize */ F0_1.ps_partkey AS ps_partkey, F0_1.ps_suppkey AS ps_suppkey, F0_1.ps_availqty AS ps_availqty, F0_1.ps_supplycost AS ps_supplycost, F0_1.ps_comment AS ps_comment, F0_1.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_1.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_1.left__result_tid AS left__result_tid, F0_1.left__setprov_dup_count AS left__setprov_dup_count, F1_1."nesting_eval_1" AS "nesting_eval_1", F1_1.prov_part_p__partkey AS prov_part_p__partkey, F1_1.right__result_tid AS right__result_tid, F1_1.right__setprov_dup_count AS right__setprov_dup_count, _mergerowid(F0_1.left__result_tid, F1_1.right__result_tid) AS _result_tid, greatest(F0_1.left__setprov_dup_count, F1_1.right__setprov_dup_count) AS _setprov_dup_count
FROM (
SELECT F0_1.ps_partkey AS ps_partkey, F0_1.ps_suppkey AS ps_suppkey, F0_1.ps_availqty AS ps_availqty, F0_1.ps_supplycost AS ps_supplycost, F0_1.ps_comment AS ps_comment, F0_1.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_1.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_1._result_tid AS left__result_tid, F0_1._setprov_dup_count AS left__setprov_dup_count
FROM (SELECT * FROM temp_view_9) F0_1) F0_1, LATERAL (
SELECT F0_2."nesting_eval_1" AS "nesting_eval_1", F0_2.prov_part_p__partkey AS prov_part_p__partkey, F0_2._result_tid AS right__result_tid, F0_2._setprov_dup_count AS right__setprov_dup_count
FROM (
SELECT /*+ materialize */ (CASE  WHEN ((F0_2."nesting_eval_1") IS NULL) THEN FALSE WHEN (F0_2."nesting_eval_1" = 1) THEN (NULL)::bool ELSE (F0_2."nesting_eval_1" = 2) END) AS "nesting_eval_1", F0_2.prov_part_p__partkey AS prov_part_p__partkey, F0_2._result_tid AS _result_tid, F0_2._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT /*+ materialize */ F0_2."nesting_eval_1" AS "nesting_eval_1", F1_2.prov_part_p__partkey AS prov_part_p__partkey, 1 AS _result_tid, ROW_NUMBER() OVER () AS _setprov_dup_count
FROM ((
SELECT max((CASE  WHEN (F0_1.ps_partkey = F0_2.p_partkey) THEN 2 WHEN (((F0_1.ps_partkey) IS NULL) OR ((F0_2.p_partkey) IS NULL)) THEN 1 ELSE 0 END)) AS "nesting_eval_1"
FROM (SELECT * FROM temp_view_10) F0_2) F0_2 LEFT OUTER JOIN (
SELECT (CASE  WHEN (F0_1.ps_partkey = F0_2.p_partkey) THEN 2 WHEN (((F0_1.ps_partkey) IS NULL) OR ((F0_2.p_partkey) IS NULL)) THEN 1 ELSE 0 END) AS nesting_eval_help, F0_2.prov_part_p__partkey AS prov_part_p__partkey, F0_2._result_tid AS _result_tid, F0_2._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_11) F0_2) F1_2 ON ((1 = 1)))) F0_2) F0_2) F1_1),
temp_view_7 AS (
SELECT /*+ materialize */ F0_1.ps_partkey AS ps_partkey, F0_1.ps_suppkey AS ps_suppkey, F0_1.ps_availqty AS ps_availqty, F0_1.ps_supplycost AS ps_supplycost, F0_1.ps_comment AS ps_comment, F0_1."nesting_eval_1" AS "nesting_eval_1", F0_1.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_1.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_1.prov_part_p__partkey AS prov_part_p__partkey, F0_1._result_tid AS _result_tid, F0_1._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_8) F0_1),
temp_view_6 AS (
SELECT /*+ materialize */ F0_1.ps_partkey AS ps_partkey, F0_1.ps_suppkey AS ps_suppkey, F0_1.ps_availqty AS ps_availqty, F0_1.ps_supplycost AS ps_supplycost, F0_1.ps_comment AS ps_comment, F0_1."nesting_eval_1" AS "nesting_eval_1", F0_1.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_1.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_1.prov_part_p__partkey AS prov_part_p__partkey, F0_1.left__result_tid AS left__result_tid, F0_1.left__setprov_dup_count AS left__setprov_dup_count, F1_1."nesting_eval_2" AS "nesting_eval_2", F1_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_1.right__result_tid AS right__result_tid, F1_1.right__setprov_dup_count AS right__setprov_dup_count, _mergerowid(F0_1.left__result_tid, F1_1.right__result_tid) AS _result_tid, greatest(F0_1.left__setprov_dup_count, F1_1.right__setprov_dup_count) AS _setprov_dup_count
FROM (
SELECT F0_1.ps_partkey AS ps_partkey, F0_1.ps_suppkey AS ps_suppkey, F0_1.ps_availqty AS ps_availqty, F0_1.ps_supplycost AS ps_supplycost, F0_1.ps_comment AS ps_comment, F0_1."nesting_eval_1" AS "nesting_eval_1", F0_1.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_1.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_1.prov_part_p__partkey AS prov_part_p__partkey, F0_1._result_tid AS left__result_tid, F0_1._setprov_dup_count AS left__setprov_dup_count
FROM (SELECT * FROM temp_view_7) F0_1) F0_1, LATERAL (
SELECT F0_2."nesting_eval_2" AS "nesting_eval_2", F0_2.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_2.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_2._result_tid AS right__result_tid, F0_2._setprov_dup_count AS right__setprov_dup_count
FROM (
SELECT /*+ materialize */ F0_2."(0500000*sum(l_quantity))" AS "nesting_eval_2", F0_2.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_2.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_2._result_tid AS _result_tid, F0_2._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT /*+ materialize */ (0.500000 * F0_2."AGGR_0") AS "(0500000*sum(l_quantity))", F0_2.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_2.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_2._result_tid AS _result_tid, F0_2._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT /*+ materialize */ F0_2."AGGR_0" AS "AGGR_0", F1_2.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_2.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, 1 AS _result_tid, ROW_NUMBER() OVER () AS _setprov_dup_count
FROM ((
SELECT sum(F0_2.l_quantity) AS "AGGR_0"
FROM lineitem F0_2
WHERE ((((F0_2.l_partkey = F0_1.ps_partkey) AND (F0_2.l_suppkey = F0_1.ps_suppkey)) AND (F0_2.l_shipdate >= '1994-01-01')) AND (F0_2.l_shipdate < '1995-01-01'))) F0_2 LEFT OUTER JOIN (
SELECT F0_2.l_orderkey AS l_orderkey, F0_2.l_partkey AS l_partkey, F0_2.l_suppkey AS l_suppkey, F0_2.l_linenumber AS l_linenumber, F0_2.l_quantity AS l_quantity, F0_2.l_extendedprice AS l_extendedprice, F0_2.l_discount AS l_discount, F0_2.l_tax AS l_tax, F0_2.l_returnflag AS l_returnflag, F0_2.l_linestatus AS l_linestatus, F0_2.l_shipdate AS l_shipdate, F0_2.l_commitdate AS l_commitdate, F0_2.l_receiptdate AS l_receiptdate, F0_2.l_shipinstruct AS l_shipinstruct, F0_2.l_shipmode AS l_shipmode, F0_2.l_comment AS l_comment, F0_2.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_2.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_2._result_tid AS _result_tid, F0_2._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT F0_2.l_orderkey AS l_orderkey, F0_2.l_partkey AS l_partkey, F0_2.l_suppkey AS l_suppkey, F0_2.l_linenumber AS l_linenumber, F0_2.l_quantity AS l_quantity, F0_2.l_extendedprice AS l_extendedprice, F0_2.l_discount AS l_discount, F0_2.l_tax AS l_tax, F0_2.l_returnflag AS l_returnflag, F0_2.l_linestatus AS l_linestatus, F0_2.l_shipdate AS l_shipdate, F0_2.l_commitdate AS l_commitdate, F0_2.l_receiptdate AS l_receiptdate, F0_2.l_shipinstruct AS l_shipinstruct, F0_2.l_shipmode AS l_shipmode, F0_2.l_comment AS l_comment, F0_2.l_orderkey AS prov_lineitem_l__orderkey, F0_2.l_linenumber AS prov_lineitem_l__linenumber, _tid2int8(F0_2.ctid) AS _result_tid, 1 AS _setprov_dup_count
FROM lineitem F0_2) F0_2
WHERE ((((F0_2.l_partkey = F0_1.ps_partkey) AND (F0_2.l_suppkey = F0_1.ps_suppkey)) AND (F0_2.l_shipdate >= '1994-01-01')) AND (F0_2.l_shipdate < '1995-01-01'))) F1_2 ON ((1 = 1)))) F0_2) F0_2) F0_2) F1_1),
temp_view_5 AS (
SELECT /*+ materialize */ F0_1.ps_suppkey AS ps_suppkey, F0_1.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_1.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_1.prov_part_p__partkey AS prov_part_p__partkey, F0_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_1._result_tid AS _result_tid, F0_1._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT F0_1.ps_partkey AS ps_partkey, F0_1.ps_suppkey AS ps_suppkey, F0_1.ps_availqty AS ps_availqty, F0_1.ps_supplycost AS ps_supplycost, F0_1.ps_comment AS ps_comment, F0_1."nesting_eval_1" AS "nesting_eval_1", F0_1."nesting_eval_2" AS "nesting_eval_2", F0_1.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_1.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_1.prov_part_p__partkey AS prov_part_p__partkey, F0_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_1._result_tid AS _result_tid, F0_1._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_6) F0_1) F0_1
WHERE (F0_1."nesting_eval_1" AND (F0_1.ps_availqty > F0_1."nesting_eval_2"))),
temp_view_1 AS (
SELECT /*+ materialize */ F0_0.s_suppkey AS s_suppkey, F0_0.s_name AS s_name, F0_0.s_address AS s_address, F0_0.s_nationkey AS s_nationkey, F0_0.s_phone AS s_phone, F0_0.s_acctbal AS s_acctbal, F0_0.s_comment AS s_comment, F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_regionkey AS n_regionkey, F0_0.n_comment AS n_comment, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0.left__result_tid AS left__result_tid, F0_0.left__setprov_dup_count AS left__setprov_dup_count, F1_0."nesting_eval_3" AS "nesting_eval_3", F1_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F1_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F1_0.prov_part_p__partkey AS prov_part_p__partkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.right__result_tid AS right__result_tid, F1_0.right__setprov_dup_count AS right__setprov_dup_count, _mergerowid(F0_0.left__result_tid, F1_0.right__result_tid) AS _result_tid, greatest(F0_0.left__setprov_dup_count, F1_0.right__setprov_dup_count) AS _setprov_dup_count
FROM (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_name AS s_name, F0_0.s_address AS s_address, F0_0.s_nationkey AS s_nationkey, F0_0.s_phone AS s_phone, F0_0.s_acctbal AS s_acctbal, F0_0.s_comment AS s_comment, F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_regionkey AS n_regionkey, F0_0.n_comment AS n_comment, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0._result_tid AS left__result_tid, F0_0._setprov_dup_count AS left__setprov_dup_count
FROM (SELECT * FROM temp_view_2) F0_0) F0_0, LATERAL (
SELECT F0_1."nesting_eval_3" AS "nesting_eval_3", F0_1.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_1.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_1.prov_part_p__partkey AS prov_part_p__partkey, F0_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_1._result_tid AS right__result_tid, F0_1._setprov_dup_count AS right__setprov_dup_count
FROM (
SELECT /*+ materialize */ (CASE  WHEN ((F0_1."nesting_eval_3") IS NULL) THEN FALSE WHEN (F0_1."nesting_eval_3" = 1) THEN (NULL)::bool ELSE (F0_1."nesting_eval_3" = 2) END) AS "nesting_eval_3", F0_1.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_1.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_1.prov_part_p__partkey AS prov_part_p__partkey, F0_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_1._result_tid AS _result_tid, F0_1._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT /*+ materialize */ F0_1."nesting_eval_3" AS "nesting_eval_3", F1_1.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F1_1.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F1_1.prov_part_p__partkey AS prov_part_p__partkey, F1_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, 1 AS _result_tid, ROW_NUMBER() OVER () AS _setprov_dup_count
FROM ((
SELECT max((CASE  WHEN (F0_0.s_suppkey = F0_1.ps_suppkey) THEN 2 WHEN (((F0_0.s_suppkey) IS NULL) OR ((F0_1.ps_suppkey) IS NULL)) THEN 1 ELSE 0 END)) AS "nesting_eval_3"
FROM (SELECT * FROM temp_view_3) F0_1) F0_1 LEFT OUTER JOIN (
SELECT (CASE  WHEN (F0_0.s_suppkey = F0_1.ps_suppkey) THEN 2 WHEN (((F0_0.s_suppkey) IS NULL) OR ((F0_1.ps_suppkey) IS NULL)) THEN 1 ELSE 0 END) AS nesting_eval_help, F0_1.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_1.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_1.prov_part_p__partkey AS prov_part_p__partkey, F0_1.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_1.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_1._result_tid AS _result_tid, F0_1._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_5) F0_1) F1_1 ON ((1 = 1)))) F0_1) F0_1) F1_0),
temp_view_0 AS (
SELECT /*+ materialize */ F0_0.s_name AS s_name, F0_0.s_address AS s_address, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.s_name AS s_name, F0_0.s_address AS s_address, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_name AS s_name, F0_0.s_address AS s_address, F0_0.s_nationkey AS s_nationkey, F0_0.s_phone AS s_phone, F0_0.s_acctbal AS s_acctbal, F0_0.s_comment AS s_comment, F0_0.n_nationkey AS n_nationkey, F0_0.n_name AS n_name, F0_0.n_regionkey AS n_regionkey, F0_0.n_comment AS n_comment, F0_0."nesting_eval_3" AS "nesting_eval_3", F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_1) F0_0) F0_0
WHERE ((F0_0."nesting_eval_3" AND (F0_0.s_nationkey = F0_0.n_nationkey)) AND (F0_0.n_name = 'CANADA'))
ORDER BY s_name ASC NULLS LAST) F0_0)
SELECT F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_nation_n__nationkey AS prov_nation_n__nationkey, F0_0.prov_partsupp_ps__partkey AS prov_partsupp_ps__partkey, F0_0.prov_partsupp_ps__suppkey AS prov_partsupp_ps__suppkey, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.s_name AS s_name, F0_0.s_address AS s_address
FROM (SELECT * FROM temp_view_0) F0_0;


