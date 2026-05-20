WITH temp_view_1 AS (
SELECT /*+ materialize */ F0_0.c_custkey AS c_custkey, count(F1_0.o_orderkey) AS c_count
FROM (customer F0_0 LEFT OUTER JOIN orders F1_0 ON (((F0_0.c_custkey = F1_0.o_custkey) AND (NOT ((F1_0.o_comment LIKE '%special%requests%'))))))
GROUP BY F0_0.c_custkey),
temp_view_5 AS (
SELECT /*+ materialize */ F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_mktsegment AS c_mktsegment, F0_0.c_comment AS c_comment, F1_0.o_orderkey AS o_orderkey, F1_0.o_custkey AS o_custkey, F1_0.o_orderstatus AS o_orderstatus, F1_0.o_totalprice AS o_totalprice, F1_0.o_orderdate AS o_orderdate, F1_0.o_orderpriority AS o_orderpriority, F1_0.o_clerk AS o_clerk, F1_0.o_shippriority AS o_shippriority, F1_0.o_comment AS o_comment, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT F0_0.c_custkey AS c_custkey, F0_0.c_name AS c_name, F0_0.c_address AS c_address, F0_0.c_nationkey AS c_nationkey, F0_0.c_phone AS c_phone, F0_0.c_acctbal AS c_acctbal, F0_0.c_mktsegment AS c_mktsegment, F0_0.c_comment AS c_comment, F0_0.c_custkey AS prov_customer_c__custkey
FROM customer F0_0) F0_0 LEFT OUTER JOIN (
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_custkey AS o_custkey, F0_0.o_orderstatus AS o_orderstatus, F0_0.o_totalprice AS o_totalprice, F0_0.o_orderdate AS o_orderdate, F0_0.o_orderpriority AS o_orderpriority, F0_0.o_clerk AS o_clerk, F0_0.o_shippriority AS o_shippriority, F0_0.o_comment AS o_comment, F0_0.o_orderkey AS prov_orders_o__orderkey
FROM orders F0_0) F1_0 ON (((F0_0.c_custkey = F1_0.o_custkey) AND (NOT ((F1_0.o_comment LIKE '%special%requests%'))))))),
temp_view_4 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0", F0_0."GROUP_0" AS "GROUP_0", F1_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT count(F1_0.o_orderkey) AS "AGGR_0", F0_0.c_custkey AS "GROUP_0"
FROM (customer F0_0 LEFT OUTER JOIN orders F1_0 ON (((F0_0.c_custkey = F1_0.o_custkey) AND (NOT ((F1_0.o_comment LIKE '%special%requests%'))))))
GROUP BY F0_0.c_custkey) F0_0 JOIN (
SELECT F0_0.c_custkey AS "_P_SIDE_GROUP_0", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM (SELECT * FROM temp_view_5) F0_0) F1_0 ON ((F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")))),
temp_view_3 AS (
SELECT /*+ materialize */ F0_0."GROUP_0" AS c_custkey, F0_0."AGGR_0" AS c_count, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM (SELECT * FROM temp_view_4) F0_0),
temp_view_2 AS (
SELECT /*+ materialize */ 1 AS "AGG_GB_ARG0", F0_0.c_count AS "AGG_GB_ARG1", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM (SELECT * FROM temp_view_3) F0_0),
temp_view_0 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0", F0_0."GROUP_0" AS "GROUP_0", F1_0.prov_customer_c__custkey AS prov_customer_c__custkey, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM ((
SELECT count(1) AS "AGGR_0", F0_0.c_count AS "GROUP_0"
FROM (SELECT * FROM temp_view_1) F0_0
GROUP BY F0_0.c_count) F0_0 JOIN (
SELECT F0_0."AGG_GB_ARG1" AS "_P_SIDE_GROUP_0", F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM (SELECT * FROM temp_view_2) F0_0) F1_0 ON ((F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0"))))
SELECT F0_0."GROUP_0" AS c_count, F0_0."AGGR_0" AS custdist, F0_0.prov_customer_c__custkey AS prov_customer_c__custkey, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey
FROM (SELECT * FROM temp_view_0) F0_0
ORDER BY custdist DESC NULLS LAST, c_count DESC NULLS LAST;


