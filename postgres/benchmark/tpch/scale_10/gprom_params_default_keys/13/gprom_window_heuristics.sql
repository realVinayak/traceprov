
SELECT F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.c_count AS c_count
FROM (
SELECT F0_0."AGGR_0" AS c_count, count((CASE  WHEN (1 = F0_0._setprov_dup_count) THEN 1 ELSE (NULL)::int8 END)) OVER (PARTITION BY F0_0."AGGR_0") AS custdist, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM (
SELECT F0_0.c_custkey AS c_custkey, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0."AGGR_0" AS "AGGR_0", row_number() OVER (PARTITION BY F0_0.c_custkey ORDER BY F0_0.c_custkey) AS _setprov_dup_count
FROM (
SELECT F0_0.c_custkey AS c_custkey, F1_0.o_orderkey AS o_orderkey, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, count(F1_0.o_orderkey) OVER (PARTITION BY F0_0.c_custkey) AS "AGGR_0"
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_custkey AS prov_customer_c__custkey
FROM customer F0_0) F0_0 LEFT OUTER JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_comment AS o_comment, F0_0.o_orderkey AS prov_orders_o__orderkey
FROM orders F0_0) F1_0 ON (((F0_0.c_custkey = F1_0.o_custkey) AND (NOT ((F1_0.o_comment LIKE '%special%requests%'))))))) F0_0) F0_0
ORDER BY custdist DESC NULLS LAST, c_count DESC NULLS LAST) F0_0;


