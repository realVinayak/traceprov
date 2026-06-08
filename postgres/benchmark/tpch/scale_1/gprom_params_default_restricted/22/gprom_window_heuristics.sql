
SELECT F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM (
SELECT substr(F0_0.c_phone, 1, 2) AS cntrycode, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM (
SELECT F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0."nesting_eval_1" AS "nesting_eval_1", F1_0."nesting_eval_2" AS "nesting_eval_2", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", greatest(F0_0.left__setprov_dup_count, F1_0.right__setprov_dup_count) AS _setprov_dup_count
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F1_0."nesting_eval_1" AS "nesting_eval_1", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", greatest(F0_0.left__setprov_dup_count, F1_0.right__setprov_dup_count) AS left__setprov_dup_count
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_custkey AS prov_customer_c__custkey, 1 AS left__setprov_dup_count
FROM customer F0_0) F0_0, LATERAL (
SELECT F0_1."AGGR_0" AS "nesting_eval_1", F0_1."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", ROW_NUMBER() OVER () AS right__setprov_dup_count
FROM (
SELECT F0_1.c_acctbal AS c_acctbal, F0_1."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_1._result_tid AS _result_tid, avg(F0_1.c_acctbal) OVER () AS "AGGR_0", count(1) OVER () AS __dummy_cnt
FROM ((
SELECT F0_1.c_acctbal AS c_acctbal, F0_1."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey", F0_1._result_tid AS _result_tid
FROM (
SELECT F0_1.c_phone AS c_phone, F0_1.c_acctbal AS c_acctbal, F0_1.c_custkey AS "prov_customer_1_c__custkey", (F0_1.c_custkey)::int8 AS _result_tid
FROM customer F0_1) F0_1
WHERE ((F0_1.c_acctbal > 0.000000) AND substr(F0_1.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17')) UNION ALL (SELECT NULL AS c_acctbal, NULL AS "prov_customer_1_c__custkey", -1 AS _result_tid))) F0_1) F0_1
WHERE ((F0_1.__dummy_cnt = 1) OR (F0_1._result_tid <> -1))) F1_0) F0_0, LATERAL (
SELECT (F0_1."aggr_0" > 0) AS "nesting_eval_2", ROW_NUMBER() OVER () AS right__setprov_dup_count
FROM (
SELECT F0_1._result_tid AS _result_tid, count(1) OVER () AS "aggr_0"
FROM ((
SELECT F0_1._result_tid AS _result_tid
FROM (
SELECT F0_1.o_custkey AS o_custkey, (F0_1.o_orderkey)::int8 AS _result_tid
FROM orders F0_1) F0_1
WHERE (F0_1.o_custkey = F0_0.c_custkey) UNION ALL (SELECT -1 AS _result_tid))) F0_1) F0_1
WHERE ((F0_1."aggr_0" = 0) OR (F0_1._result_tid <> -1))) F1_0) F0_0
WHERE ((substr(F0_0.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17') AND (F0_0.c_acctbal > F0_0."nesting_eval_1")) AND (NOT (F0_0."nesting_eval_2")))
ORDER BY cntrycode ASC NULLS LAST) F0_0;


