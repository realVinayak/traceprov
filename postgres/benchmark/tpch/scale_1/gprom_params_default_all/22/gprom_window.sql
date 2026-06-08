WITH temp_view_5 AS (
SELECT /*+ materialize */ F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_mktsegment AS c_mktsegment, F0_0.c_comment AS c_comment, F0_0.c_custkey AS prov_customer_c__custkey, (F0_0.c_custkey)::int8 AS _result_tid, 1 AS _setprov_dup_count
FROM customer F0_0),
temp_view_8 AS (
SELECT /*+ materialize */ F0_1."AGGR_0" AS "AGGR_0", F0_1."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", 1 AS _result_tid, ROW_NUMBER() OVER () AS _setprov_dup_count
FROM (
SELECT F0_1.c_custkey AS c_custkey, F0_1.c_name AS c_name, F0_1.c_address AS c_address, F0_1.c_nationkey AS c_nationkey, F0_1.c_phone AS c_phone, F0_1.c_acctbal AS c_acctbal, F0_1.c_mktsegment AS c_mktsegment, F0_1.c_comment AS c_comment, F0_1."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_1._result_tid AS _result_tid, F0_1._setprov_dup_count AS _setprov_dup_count, avg(F0_1.c_acctbal) OVER () AS "AGGR_0", count(1) OVER () AS __dummy_cnt
FROM ((
SELECT F0_1.c_custkey AS c_custkey, F0_1.c_name AS c_name, F0_1.c_address AS c_address, F0_1.c_nationkey AS c_nationkey, F0_1.c_phone AS c_phone, F0_1.c_acctbal AS c_acctbal, F0_1.c_mktsegment AS c_mktsegment, F0_1.c_comment AS c_comment, F0_1."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_1._result_tid AS _result_tid, F0_1._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT F0_1.c_custkey AS c_custkey, F0_1.c_name AS c_name, F0_1.c_address AS c_address, F0_1.c_nationkey AS c_nationkey, F0_1.c_phone AS c_phone, F0_1.c_acctbal AS c_acctbal, F0_1.c_mktsegment AS c_mktsegment, F0_1.c_comment AS c_comment, F0_1.c_custkey AS "prov_customer_1_c__custkey", (F0_1.c_custkey)::int8 AS _result_tid, 1 AS _setprov_dup_count
FROM customer F0_1) F0_1
WHERE ((F0_1.c_acctbal > 0.000000) AND substr(F0_1.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17')) UNION ALL (SELECT NULL AS c_custkey, NULL AS c_name, NULL AS c_address, NULL AS c_nationkey, NULL AS c_phone, NULL AS c_acctbal, NULL AS c_mktsegment, NULL AS c_comment, NULL AS "prov_customer_1_c__custkey", -1 AS _result_tid, NULL AS _setprov_dup_count))) F0_1) F0_1
WHERE ((F0_1.__dummy_cnt = 1) OR (F0_1._result_tid <> -1))),
temp_view_7 AS (
SELECT /*+ materialize */ F0_1."AGGR_0" AS "avg(c_acctbal)", F0_1."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_1._result_tid AS _result_tid, F0_1._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_8) F0_1),
temp_view_6 AS (
SELECT /*+ materialize */ F0_1."avg(c_acctbal)" AS "nesting_eval_1", F0_1."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_1._result_tid AS _result_tid, F0_1._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_7) F0_1),
temp_view_4 AS (
SELECT /*+ materialize */ F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_mktsegment AS c_mktsegment, F0_0.c_comment AS c_comment, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.left__result_tid AS left__result_tid, F0_0.left__setprov_dup_count AS left__setprov_dup_count, F1_0."nesting_eval_1" AS "nesting_eval_1", F1_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F1_0.right__result_tid AS right__result_tid, F1_0.right__setprov_dup_count AS right__setprov_dup_count, _mergerowid(F0_0.left__result_tid, F1_0.right__result_tid) AS _result_tid, greatest(F0_0.left__setprov_dup_count, F1_0.right__setprov_dup_count) AS _setprov_dup_count
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_mktsegment AS c_mktsegment, F0_0.c_comment AS c_comment, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0._result_tid AS left__result_tid, F0_0._setprov_dup_count AS left__setprov_dup_count
FROM (SELECT * FROM temp_view_5) F0_0) F0_0, LATERAL (
SELECT F0_1."nesting_eval_1" AS "nesting_eval_1", F0_1."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_1._result_tid AS right__result_tid, F0_1._setprov_dup_count AS right__setprov_dup_count
FROM (SELECT * FROM temp_view_6) F0_1) F1_0),
temp_view_3 AS (
SELECT /*+ materialize */ F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_mktsegment AS c_mktsegment, F0_0.c_comment AS c_comment, F0_0."nesting_eval_1" AS "nesting_eval_1", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_4) F0_0),
temp_view_2 AS (
SELECT /*+ materialize */ F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_mktsegment AS c_mktsegment, F0_0.c_comment AS c_comment, F0_0."nesting_eval_1" AS "nesting_eval_1", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_0.left__result_tid AS left__result_tid, F0_0.left__setprov_dup_count AS left__setprov_dup_count, F1_0."nesting_eval_2" AS "nesting_eval_2", F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.right__result_tid AS right__result_tid, F1_0.right__setprov_dup_count AS right__setprov_dup_count, _mergerowid(F0_0.left__result_tid, F1_0.right__result_tid) AS _result_tid, greatest(F0_0.left__setprov_dup_count, F1_0.right__setprov_dup_count) AS _setprov_dup_count
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_mktsegment AS c_mktsegment, F0_0.c_comment AS c_comment, F0_0."nesting_eval_1" AS "nesting_eval_1", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_0._result_tid AS left__result_tid, F0_0._setprov_dup_count AS left__setprov_dup_count
FROM (SELECT * FROM temp_view_3) F0_0) F0_0, LATERAL (
SELECT F0_1."nesting_eval_2" AS "nesting_eval_2", F0_1.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_1._result_tid AS right__result_tid, F0_1._setprov_dup_count AS right__setprov_dup_count
FROM (
SELECT /*+ materialize */ (F0_1."aggr_0" > 0) AS "nesting_eval_2", F0_1.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_1._result_tid AS _result_tid, F0_1._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT /*+ materialize */ F0_1."aggr_0" AS "aggr_0", F0_1.prov_orders_o__orderkey AS prov_orders_o__orderkey, 1 AS _result_tid, ROW_NUMBER() OVER () AS _setprov_dup_count
FROM (
SELECT F0_1.o_orderkey AS o_orderkey, F0_1.o_custkey AS o_custkey, F0_1.o_orderstatus AS o_orderstatus, F0_1.o_totalprice AS o_totalprice, F0_1.o_orderdate AS o_orderdate, F0_1.o_orderpriority AS o_orderpriority, F0_1.o_clerk AS o_clerk, F0_1.o_shippriority AS o_shippriority, F0_1.o_comment AS o_comment, F0_1.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_1._result_tid AS _result_tid, F0_1._setprov_dup_count AS _setprov_dup_count, count(1) OVER () AS "aggr_0"
FROM ((
SELECT F0_1.o_orderkey AS o_orderkey, F0_1.o_custkey AS o_custkey, F0_1.o_orderstatus AS o_orderstatus, F0_1.o_totalprice AS o_totalprice, F0_1.o_orderdate AS o_orderdate, F0_1.o_orderpriority AS o_orderpriority, F0_1.o_clerk AS o_clerk, F0_1.o_shippriority AS o_shippriority, F0_1.o_comment AS o_comment, F0_1.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_1._result_tid AS _result_tid, F0_1._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT F0_1.o_orderkey AS o_orderkey, F0_1.o_custkey AS o_custkey, F0_1.o_orderstatus AS o_orderstatus, F0_1.o_totalprice AS o_totalprice, F0_1.o_orderdate AS o_orderdate, F0_1.o_orderpriority AS o_orderpriority, F0_1.o_clerk AS o_clerk, F0_1.o_shippriority AS o_shippriority, F0_1.o_comment AS o_comment, F0_1.o_orderkey AS prov_orders_o__orderkey, (F0_1.o_orderkey)::int8 AS _result_tid, 1 AS _setprov_dup_count
FROM orders F0_1) F0_1
WHERE (F0_1.o_custkey = F0_0.c_custkey) UNION ALL (SELECT NULL AS o_orderkey, NULL AS o_custkey, NULL AS o_orderstatus, NULL AS o_totalprice, NULL AS o_orderdate, NULL AS o_orderpriority, NULL AS o_clerk, NULL AS o_shippriority, NULL AS o_comment, NULL AS prov_orders_o__orderkey, -1 AS _result_tid, NULL AS _setprov_dup_count))) F0_1) F0_1
WHERE ((F0_1."aggr_0" = 0) OR (F0_1._result_tid <> -1))) F0_1) F0_1) F1_0),
temp_view_1 AS (
SELECT /*+ materialize */ substr(F0_0.c_phone, 1, 2) AS cntrycode, F0_0.c_acctbal AS c_acctbal, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_mktsegment AS c_mktsegment, F0_0.c_comment AS c_comment, F0_0."nesting_eval_1" AS "nesting_eval_1", F0_0."nesting_eval_2" AS "nesting_eval_2", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_2) F0_0) F0_0
WHERE ((substr(F0_0.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17') AND (F0_0.c_acctbal > F0_0."nesting_eval_1")) AND (NOT (F0_0."nesting_eval_2")))),
temp_view_0 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0", F0_0."AGGR_1" AS "AGGR_1", F0_0."AGG_GB_ARG2" AS "GROUP_0", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, dense_rank() OVER ( ORDER BY F0_0."AGG_GB_ARG2") AS _result_tid, row_number() OVER (PARTITION BY F0_0."AGG_GB_ARG2" ORDER BY F0_0."AGG_GB_ARG2") AS _setprov_dup_count
FROM (
SELECT 1 AS "AGG_GB_ARG0", F0_0.c_acctbal AS "AGG_GB_ARG1", F0_0.cntrycode AS "AGG_GB_ARG2", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count, count((CASE  WHEN (1 = F0_0._setprov_dup_count) THEN 1 ELSE (NULL)::int8 END)) OVER (PARTITION BY F0_0.cntrycode) AS "AGGR_0", sum((CASE  WHEN (1 = F0_0._setprov_dup_count) THEN F0_0.c_acctbal ELSE (NULL)::int8 END)) OVER (PARTITION BY F0_0.cntrycode) AS "AGGR_1"
FROM (SELECT * FROM temp_view_1) F0_0) F0_0)
SELECT F0_0.cntrycode AS cntrycode, F0_0.numcust AS numcust, F0_0.totacctbal AS totacctbal, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM (
SELECT F0_0."GROUP_0" AS cntrycode, F0_0."AGGR_0" AS numcust, F0_0."AGGR_1" AS totacctbal, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_0) F0_0
ORDER BY cntrycode ASC NULLS LAST) F0_0;


