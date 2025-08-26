WITH _temp_view_2 AS (
SELECT /*+ materialize */ sum(F0."l_quantity") AS "AGGR_0", F0."l_orderkey" AS "GROUP_0"
FROM "lineitem" AS F0
WHERE (F0."l_orderkey" <@ '{1262992,1263046,1544617,1544675,2199678,2199741,2869147,2869204,2942437,2942491,3043267,3043325,3767242,3767309,4702729,4702787,4721981,4722041,4806720,4806784}'::int[])
GROUP BY F0."l_orderkey"),
_temp_view_1 AS (
SELECT /*+ materialize */ sum(F0."l_quantity") AS "AGGR_0", F0."c_name" AS "GROUP_0", F0."c_custkey" AS "GROUP_1", F0."o_orderkey" AS "GROUP_2", F0."o_orderdate" AS "GROUP_3", F0."o_totalprice" AS "GROUP_4"
FROM (
SELECT F0."c_custkey" AS "c_custkey", F0."c_name" AS "c_name", F0."c_address" AS "c_address", F0."c_nationkey" AS "c_nationkey", F0."c_phone" AS "c_phone", F0."c_acctbal" AS "c_acctbal", F0."c_mktsegment" AS "c_mktsegment", F0."c_comment" AS "c_comment", F0."o_orderkey" AS "o_orderkey", F0."o_custkey" AS "o_custkey", F0."o_orderstatus" AS "o_orderstatus", F0."o_totalprice" AS "o_totalprice", F0."o_orderdate" AS "o_orderdate", F0."o_orderpriority" AS "o_orderpriority", F0."o_clerk" AS "o_clerk", F0."o_shippriority" AS "o_shippriority", F0."o_comment" AS "o_comment", F0."l_orderkey" AS "l_orderkey", F0."l_partkey" AS "l_partkey", F0."l_suppkey" AS "l_suppkey", F0."l_linenumber" AS "l_linenumber", F0."l_quantity" AS "l_quantity", F0."l_extendedprice" AS "l_extendedprice", F0."l_discount" AS "l_discount", F0."l_tax" AS "l_tax", F0."l_returnflag" AS "l_returnflag", F0."l_linestatus" AS "l_linestatus", F0."l_shipdate" AS "l_shipdate", F0."l_commitdate" AS "l_commitdate", F0."l_receiptdate" AS "l_receiptdate", F0."l_shipinstruct" AS "l_shipinstruct", F0."l_shipmode" AS "l_shipmode", F0."l_comment" AS "l_comment", F1."total" AS "total", F1."l_orderkey" AS "l_orderkey1"
FROM ((
SELECT F0."c_custkey" AS "c_custkey", F0."c_name" AS "c_name", F0."c_address" AS "c_address", F0."c_nationkey" AS "c_nationkey", F0."c_phone" AS "c_phone", F0."c_acctbal" AS "c_acctbal", F0."c_mktsegment" AS "c_mktsegment", F0."c_comment" AS "c_comment", F0."o_orderkey" AS "o_orderkey", F0."o_custkey" AS "o_custkey", F0."o_orderstatus" AS "o_orderstatus", F0."o_totalprice" AS "o_totalprice", F0."o_orderdate" AS "o_orderdate", F0."o_orderpriority" AS "o_orderpriority", F0."o_clerk" AS "o_clerk", F0."o_shippriority" AS "o_shippriority", F0."o_comment" AS "o_comment", F1."l_orderkey" AS "l_orderkey", F1."l_partkey" AS "l_partkey", F1."l_suppkey" AS "l_suppkey", F1."l_linenumber" AS "l_linenumber", F1."l_quantity" AS "l_quantity", F1."l_extendedprice" AS "l_extendedprice", F1."l_discount" AS "l_discount", F1."l_tax" AS "l_tax", F1."l_returnflag" AS "l_returnflag", F1."l_linestatus" AS "l_linestatus", F1."l_shipdate" AS "l_shipdate", F1."l_commitdate" AS "l_commitdate", F1."l_receiptdate" AS "l_receiptdate", F1."l_shipinstruct" AS "l_shipinstruct", F1."l_shipmode" AS "l_shipmode", F1."l_comment" AS "l_comment"
FROM ((
SELECT F0."c_custkey" AS "c_custkey", F0."c_name" AS "c_name", F0."c_address" AS "c_address", F0."c_nationkey" AS "c_nationkey", F0."c_phone" AS "c_phone", F0."c_acctbal" AS "c_acctbal", F0."c_mktsegment" AS "c_mktsegment", F0."c_comment" AS "c_comment", F1."o_orderkey" AS "o_orderkey", F1."o_custkey" AS "o_custkey", F1."o_orderstatus" AS "o_orderstatus", F1."o_totalprice" AS "o_totalprice", F1."o_orderdate" AS "o_orderdate", F1."o_orderpriority" AS "o_orderpriority", F1."o_clerk" AS "o_clerk", F1."o_shippriority" AS "o_shippriority", F1."o_comment" AS "o_comment"
FROM ((
SELECT F0."c_custkey" AS "c_custkey", F0."c_name" AS "c_name", F0."c_address" AS "c_address", F0."c_nationkey" AS "c_nationkey", F0."c_phone" AS "c_phone", F0."c_acctbal" AS "c_acctbal", F0."c_mktsegment" AS "c_mktsegment", F0."c_comment" AS "c_comment"
FROM "customer" AS F0
WHERE (F0."c_custkey" <@ '{15619,15620,36619,36620,66789,66795,117919,117920,119989,119990,126864,126870,128120,128121,144617,144618,147197,147198,148884,148890}'::int[])) F0 CROSS JOIN (
SELECT F0."o_orderkey" AS "o_orderkey", F0."o_custkey" AS "o_custkey", F0."o_orderstatus" AS "o_orderstatus", F0."o_totalprice" AS "o_totalprice", F0."o_orderdate" AS "o_orderdate", F0."o_orderpriority" AS "o_orderpriority", F0."o_clerk" AS "o_clerk", F0."o_shippriority" AS "o_shippriority", F0."o_comment" AS "o_comment"
FROM "orders" AS F0
WHERE (F0."o_orderkey" <@ '{1262982,1263042,1544628,1544688,2199708,2199768,2869122,2869188,2942458,2942516,3043258,3043316,3767268,3767328,4702722,4702788,4721988,4722048,4806714,4806774}'::int[])) F1)) F0 CROSS JOIN (
SELECT F0."l_orderkey" AS "l_orderkey", F0."l_partkey" AS "l_partkey", F0."l_suppkey" AS "l_suppkey", F0."l_linenumber" AS "l_linenumber", F0."l_quantity" AS "l_quantity", F0."l_extendedprice" AS "l_extendedprice", F0."l_discount" AS "l_discount", F0."l_tax" AS "l_tax", F0."l_returnflag" AS "l_returnflag", F0."l_linestatus" AS "l_linestatus", F0."l_shipdate" AS "l_shipdate", F0."l_commitdate" AS "l_commitdate", F0."l_receiptdate" AS "l_receiptdate", F0."l_shipinstruct" AS "l_shipinstruct", F0."l_shipmode" AS "l_shipmode", F0."l_comment" AS "l_comment"
FROM "lineitem" AS F0
WHERE (F0."l_orderkey" <@ '{1262992,1263046,1544617,1544675,2199678,2199741,2869147,2869204,2942437,2942491,3043267,3043325,3767242,3767309,4702729,4702787,4721981,4722041,4806720,4806784}'::int[])) F1)) F0 CROSS JOIN (
SELECT F0."total" AS "total", F0."l_orderkey" AS "l_orderkey"
FROM (
SELECT F0."AGGR_0" AS "total", F0."GROUP_0" AS "l_orderkey"
FROM (SELECT * FROM _temp_view_2) F0) F0
WHERE (F0."total" > 312)) F1)) F0
WHERE (((F0."o_orderkey" = F0."l_orderkey1") and (F0."c_custkey" = F0."o_custkey")) and (F0."o_orderkey" = F0."l_orderkey"))
GROUP BY F0."c_name", F0."c_custkey", F0."o_orderkey", F0."o_orderdate", F0."o_totalprice"),
_temp_view_0 AS (
SELECT /*+ materialize */ F0."GROUP_0" AS "c_name", F0."GROUP_1" AS "c_custkey", F0."GROUP_2" AS "o_orderkey", F0."GROUP_3" AS "o_orderdate", F0."GROUP_4" AS "o_totalprice", F0."AGGR_0" AS "sum(l_quantity)"
FROM (SELECT * FROM _temp_view_1) F0)
SELECT F0."c_name" AS "c_name", F0."c_custkey" AS "c_custkey", F0."o_orderkey" AS "o_orderkey", F0."o_orderdate" AS "o_orderdate", F0."o_totalprice" AS "o_totalprice", F0."sum(l_quantity)" AS "sum(l_quantity)"
FROM (
SELECT F0."c_name" AS "c_name", F0."c_custkey" AS "c_custkey", F0."o_orderkey" AS "o_orderkey", F0."o_orderdate" AS "o_orderdate", F0."o_totalprice" AS "o_totalprice", F0."sum(l_quantity)" AS "sum(l_quantity)"
FROM (SELECT * FROM _temp_view_0) F0
ORDER BY F0."o_totalprice" DESC NULLS LAST, F0."o_orderdate" ASC NULLS LAST) F0
LIMIT 100;
