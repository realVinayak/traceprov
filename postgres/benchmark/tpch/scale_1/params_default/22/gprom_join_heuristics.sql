
SELECT F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM (
SELECT F0_0."GROUP_0" AS cntrycode, F1_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM ((
SELECT F0_0."AGG_GB_ARG2" AS "GROUP_0"
FROM (
SELECT DISTINCT substr(F0_0.c_phone, 1, 2) AS "AGG_GB_ARG2"
FROM (((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal
FROM customer F0_0) F0_0 CROSS JOIN (
SELECT avg(F0_0.c_acctbal) AS "avg(c_acctbal)"
FROM (
SELECT F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal
FROM customer F0_0) F0_0
WHERE ((F0_0.c_acctbal > 0.000000) AND substr(F0_0.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17'))) F1_0) CROSS JOIN ((
SELECT F0_0.c_custkey AS "c_custkey1"
FROM customer F0_0 EXCEPT ALL 
SELECT F0_0.o_custkey AS o_custkey
FROM orders F0_0)) F2_0)
WHERE ((substr(F0_0.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17') AND (F0_0.c_acctbal > F1_0."avg(c_acctbal)")) AND (F2_0."c_custkey1" = F0_0.c_custkey))) F0_0
GROUP BY F0_0."AGG_GB_ARG2") F0_0 JOIN (
SELECT substr(F0_0.c_phone, 1, 2) AS "_P_SIDE_GROUP_0", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM (
SELECT DISTINCT F0_0.c_custkey AS c_custkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0."avg(c_acctbal)" AS "avg(c_acctbal)", F1_0."c_custkey1" AS "c_custkey1", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F1_0."avg(c_acctbal)" AS "avg(c_acctbal)", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_custkey AS prov_customer_c__custkey
FROM customer F0_0) F0_0 CROSS JOIN (
SELECT F0_0."AGGR_0" AS "avg(c_acctbal)", F1_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM ((
SELECT avg(F0_0.c_acctbal) AS "AGGR_0"
FROM (
SELECT F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal
FROM customer F0_0) F0_0
WHERE ((F0_0.c_acctbal > 0.000000) AND substr(F0_0.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17'))) F0_0 LEFT OUTER JOIN (
SELECT F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0."prov_customer_1_c__custkey" AS "prov_customer_1_c__custkey"
FROM (
SELECT F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_custkey AS "prov_customer_1_c__custkey"
FROM customer F0_0) F0_0
WHERE ((F0_0.c_acctbal > 0.000000) AND substr(F0_0.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17'))) F1_0 ON ((1 = 1)))) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0."c_custkey1" AS "c_custkey1"
FROM (((
SELECT F0_0.c_custkey AS "c_custkey1"
FROM customer F0_0 EXCEPT ALL 
SELECT F0_0.o_custkey AS o_custkey
FROM orders F0_0)) F0_0 JOIN (
SELECT F0_0.c_custkey AS "c_custkey1"
FROM customer F0_0) F1_0 ON ((F0_0."c_custkey1" IS NOT DISTINCT FROM F1_0."c_custkey1")))) F1_0)) F0_0
WHERE ((substr(F0_0.c_phone, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17') AND (F0_0.c_acctbal > F0_0."avg(c_acctbal)")) AND (F0_0."c_custkey1" = F0_0.c_custkey))) F1_0 ON ((F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")))
ORDER BY cntrycode ASC NULLS LAST) F0_0;


