
SELECT F0_0."GROUP_0" AS c_count, F0_0."AGGR_0" AS custdist, F1_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT count(1) AS "AGGR_0", F0_0."AGGR_0" AS "GROUP_0"
FROM (
SELECT count(F1_0.o_orderkey) AS "AGGR_0", F0_0.c_custkey AS "GROUP_0"
FROM ((
SELECT F0_0.c_custkey AS c_custkey
FROM customer F0_0) F0_0 LEFT OUTER JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_comment AS o_comment
FROM orders F0_0) F1_0 ON (((F0_0.c_custkey = F1_0.o_custkey) AND (NOT ((F1_0.o_comment LIKE '%special%requests%'))))))
GROUP BY F0_0.c_custkey) F0_0
GROUP BY F0_0."AGGR_0") F0_0 JOIN (
SELECT F0_0."AGGR_0" AS "_P_SIDE_GROUP_0", F1_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT count(F1_0.o_orderkey) AS "AGGR_0", F0_0.c_custkey AS "GROUP_0"
FROM ((
SELECT F0_0.c_custkey AS c_custkey
FROM customer F0_0) F0_0 LEFT OUTER JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_comment AS o_comment
FROM orders F0_0) F1_0 ON (((F0_0.c_custkey = F1_0.o_custkey) AND (NOT ((F1_0.o_comment LIKE '%special%requests%'))))))
GROUP BY F0_0.c_custkey) F0_0 JOIN (
SELECT F0_0.c_custkey AS "_P_SIDE_GROUP_0", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_custkey AS prov_customer_c__custkey
FROM customer F0_0) F0_0 LEFT OUTER JOIN (
SELECT F0_0.o_custkey AS o_custkey, F0_0.o_comment AS o_comment, F0_0.o_orderkey AS prov_orders_o__orderkey
FROM orders F0_0) F1_0 ON (((F0_0.c_custkey = F1_0.o_custkey) AND (NOT ((F1_0.o_comment LIKE '%special%requests%'))))))) F1_0 ON ((F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")))) F1_0 ON ((F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")))
ORDER BY custdist DESC NULLS LAST, c_count DESC NULLS LAST;


