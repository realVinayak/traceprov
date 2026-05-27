
SELECT F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM (
SELECT F0_0."GROUP_0" AS cntrycode, F1_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM ((
SELECT substr(F0_0.c_phone, 1, 2) AS "GROUP_0"
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal
FROM customer F0_0) F0_0, LATERAL (
SELECT avg(F0_1.c_acctbal) AS "nesting_eval_1"
FROM (
SELECT F0_1.c_phone AS c_phone, F0_1.c_acctbal AS c_acctbal
FROM customer F0_1) F0_1
WHERE ((F0_1.c_acctbal > 0.000000) AND substr(F0_1.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17'))) F1_0, LATERAL (
SELECT (count(1) > 0) AS "nesting_eval_2"
FROM (
SELECT F0_1.o_custkey AS o_custkey
FROM orders F0_1) F0_1
WHERE (F0_1.o_custkey = F0_0.c_custkey)) F2_0
WHERE ((substr(F0_0.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17') AND (F0_0.c_acctbal > F1_0."nesting_eval_1")) AND (NOT (F2_0."nesting_eval_2")))
GROUP BY substr(F0_0.c_phone, 1, 2)) F0_0 JOIN (
SELECT substr(F0_0.c_phone, 1, 2) AS "_P_SIDE_GROUP_0", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM (
SELECT F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0."nesting_eval_1" AS "nesting_eval_1", F1_0."nesting_eval_2" AS "nesting_eval_2", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F1_0."nesting_eval_1" AS "nesting_eval_1", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_custkey AS prov_customer_c__custkey
FROM customer F0_0) F0_0, LATERAL (
SELECT F0_1."AGGR_0" AS "nesting_eval_1", F1_1."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM ((
SELECT avg(F0_1.c_acctbal) AS "AGGR_0"
FROM (
SELECT F0_1.c_phone AS c_phone, F0_1.c_acctbal AS c_acctbal
FROM customer F0_1) F0_1
WHERE ((F0_1.c_acctbal > 0.000000) AND substr(F0_1.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17'))) F0_1 LEFT OUTER JOIN (
SELECT F0_1.c_phone AS c_phone, F0_1.c_acctbal AS c_acctbal, F0_1."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM (
SELECT F0_1.c_phone AS c_phone, F0_1.c_acctbal AS c_acctbal, F0_1.c_custkey AS "prov_customer_1_c__custkey"
FROM customer F0_1) F0_1
WHERE ((F0_1.c_acctbal > 0.000000) AND substr(F0_1.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17'))) F1_1 ON ((1 = 1)))) F1_0) F0_0, LATERAL (
SELECT (F0_1."aggr_0" > 0) AS "nesting_eval_2"
FROM ((
SELECT count(1) AS "aggr_0"
FROM (
SELECT F0_1.o_custkey AS o_custkey
FROM orders F0_1) F0_1
WHERE (F0_1.o_custkey = F0_0.c_custkey)) F0_1 LEFT OUTER JOIN (
SELECT F0_1.o_custkey AS o_custkey
FROM (
SELECT F0_1.o_custkey AS o_custkey
FROM orders F0_1) F0_1
WHERE (F0_1.o_custkey = F0_0.c_custkey)) F1_1 ON ((1 = 1)))) F1_0) F0_0
WHERE ((substr(F0_0.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17') AND (F0_0.c_acctbal > F0_0."nesting_eval_1")) AND (NOT (F0_0."nesting_eval_2")))) F1_0 ON ((F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0")))
ORDER BY cntrycode ASC NULLS LAST) F0_0;


