WITH temp_view_2 AS (
SELECT /*+ materialize */ avg(F0_1.c_acctbal) AS "avg(c_acctbal)"
FROM customer F0_1
WHERE ((F0_1.c_acctbal > 0.000000) AND substr(F0_1.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17'))),
temp_view_1 AS (
SELECT /*+ materialize */ substr(F0_0.c_phone, 1, 2) AS cntrycode, F0_0.c_acctbal AS c_acctbal
FROM customer F0_0, LATERAL (
SELECT F0_1."avg(c_acctbal)" AS "nesting_eval_1"
FROM (SELECT * FROM temp_view_2) F0_1) F1_0, LATERAL (
SELECT (count(1) > 0) AS "nesting_eval_2"
FROM orders F0_1
WHERE (F0_1.o_custkey = F0_0.c_custkey)) F2_0
WHERE ((substr(F0_0.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17') AND (F0_0.c_acctbal > F1_0."nesting_eval_1")) AND (NOT (F2_0."nesting_eval_2")))),
temp_view_6 AS (
SELECT /*+ materialize */ F0_1."AGGR_0" AS "AGGR_0", F1_1."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM ((
SELECT avg(F0_1.c_acctbal) AS "AGGR_0"
FROM customer F0_1
WHERE ((F0_1.c_acctbal > 0.000000) AND substr(F0_1.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17'))) F0_1 LEFT OUTER JOIN (
SELECT F0_1.c_custkey AS c_custkey, F0_1.c_name AS c_name, F0_1.c_address AS c_address, F0_1.c_nationkey AS c_nationkey, F0_1.c_phone AS c_phone, F0_1.c_acctbal AS c_acctbal, F0_1.c_mktsegment AS c_mktsegment, F0_1.c_comment AS c_comment, F0_1."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM (
SELECT F0_1.c_custkey AS c_custkey, F0_1.c_name AS c_name, F0_1.c_address AS c_address, F0_1.c_nationkey AS c_nationkey, F0_1.c_phone AS c_phone, F0_1.c_acctbal AS c_acctbal, F0_1.c_mktsegment AS c_mktsegment, F0_1.c_comment AS c_comment, F0_1.c_custkey AS "prov_customer_1_c__custkey"
FROM customer F0_1) F0_1
WHERE ((F0_1.c_acctbal > 0.000000) AND substr(F0_1.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17'))) F1_1 ON ((1 = 1)))),
temp_view_5 AS (
SELECT /*+ materialize */ F0_1."AGGR_0" AS "avg(c_acctbal)", F0_1."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM (SELECT * FROM temp_view_6) F0_1),
temp_view_4 AS (
SELECT /*+ materialize */ substr(F0_0.c_phone, 1, 2) AS cntrycode, F0_0.c_acctbal AS c_acctbal, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_mktsegment AS c_mktsegment, F0_0.c_comment AS c_comment, F0_0."nesting_eval_1" AS "nesting_eval_1", F1_0."nesting_eval_2" AS "nesting_eval_2", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_mktsegment AS c_mktsegment, F0_0.c_comment AS c_comment, F1_0."nesting_eval_1" AS "nesting_eval_1", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_mktsegment AS c_mktsegment, F0_0.c_comment AS c_comment, F0_0.c_custkey AS prov_customer_c__custkey
FROM customer F0_0) F0_0, LATERAL (
SELECT F0_1."avg(c_acctbal)" AS "nesting_eval_1", F0_1."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM (SELECT * FROM temp_view_5) F0_1) F1_0) F0_0, LATERAL (
SELECT (F0_1."aggr_0" > 0) AS "nesting_eval_2", F0_1.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM (
SELECT /*+ materialize */ F0_1."aggr_0" AS "aggr_0", F1_1.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT count(1) AS "aggr_0"
FROM orders F0_1
WHERE (F0_1.o_custkey = F0_0.c_custkey)) F0_1 LEFT OUTER JOIN (
SELECT F0_1.o_orderkey AS o_orderkey, F0_1.o_custkey AS o_custkey, F0_1.o_orderstatus AS o_orderstatus, F0_1.o_totalprice AS o_totalprice, F0_1.o_orderdate AS o_orderdate, F0_1.o_orderpriority AS o_orderpriority, F0_1.o_clerk AS o_clerk, F0_1.o_shippriority AS o_shippriority, F0_1.o_comment AS o_comment, F0_1.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM (
SELECT F0_1.o_orderkey AS o_orderkey, F0_1.o_custkey AS o_custkey, F0_1.o_orderstatus AS o_orderstatus, F0_1.o_totalprice AS o_totalprice, F0_1.o_orderdate AS o_orderdate, F0_1.o_orderpriority AS o_orderpriority, F0_1.o_clerk AS o_clerk, F0_1.o_shippriority AS o_shippriority, F0_1.o_comment AS o_comment, F0_1.o_orderkey AS prov_orders_o__orderkey
FROM orders F0_1) F0_1
WHERE (F0_1.o_custkey = F0_0.c_custkey)) F1_1 ON ((1 = 1)))) F0_1) F1_0) F0_0
WHERE ((substr(F0_0.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17') AND (F0_0.c_acctbal > F0_0."nesting_eval_1")) AND (NOT (F0_0."nesting_eval_2")))),
temp_view_3 AS (
SELECT /*+ materialize */ 1 AS "AGG_GB_ARG0", F0_0.c_acctbal AS "AGG_GB_ARG1", F0_0.cntrycode AS "AGG_GB_ARG2", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM (SELECT * FROM temp_view_4) F0_0),
temp_view_0 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0", F0_0."AGGR_1" AS "AGGR_1", F0_0."GROUP_0" AS "GROUP_0", F1_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT count(1) AS "AGGR_0", sum(F0_0.c_acctbal) AS "AGGR_1", F0_0.cntrycode AS "GROUP_0"
FROM (SELECT * FROM temp_view_1) F0_0
GROUP BY F0_0.cntrycode) F0_0 JOIN (
SELECT F0_0."AGG_GB_ARG2" AS "_P_SIDE_GROUP_0", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM (SELECT * FROM temp_view_3) F0_0) F1_0 ON ((F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0"))))
SELECT F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_0.cntrycode AS cntrycode
FROM (
SELECT F0_0."GROUP_0" AS cntrycode, F0_0."AGGR_0" AS numcust, F0_0."AGGR_1" AS totacctbal, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM (SELECT * FROM temp_view_0) F0_0
ORDER BY cntrycode ASC NULLS LAST) F0_0;


