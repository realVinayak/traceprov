WITH temp_view_2 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0", F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, 1 AS _result_tid, ROW_NUMBER() OVER () AS _setprov_dup_count
FROM ((
SELECT sum((F0_0.l_extendedprice * F0_0.l_discount)) AS "AGGR_0"
FROM lineitem F0_0
WHERE (((((F0_0.l_shipdate >= '1994-01-01') AND (F0_0.l_shipdate < '1995-01-01')) AND (F0_0.l_discount >= 0.050000)) AND (F0_0.l_discount <= 0.070000)) AND (F0_0.l_quantity < 24))) F0_0 LEFT OUTER JOIN (
SELECT (F0_0.l_extendedprice * F0_0.l_discount) AS "AGG_GB_ARG0", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_linenumber AS l_linenumber, F0_0.l_quantity AS l_quantity, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_tax AS l_tax, F0_0.l_returnflag AS l_returnflag, F0_0.l_linestatus AS l_linestatus, F0_0.l_shipdate AS l_shipdate, F0_0.l_commitdate AS l_commitdate, F0_0.l_receiptdate AS l_receiptdate, F0_0.l_shipinstruct AS l_shipinstruct, F0_0.l_shipmode AS l_shipmode, F0_0.l_comment AS l_comment, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber, _tid2int8(F0_0.ctid) AS _result_tid, 1 AS _setprov_dup_count
FROM lineitem F0_0) F0_0
WHERE (((((F0_0.l_shipdate >= '1994-01-01') AND (F0_0.l_shipdate < '1995-01-01')) AND (F0_0.l_discount >= 0.050000)) AND (F0_0.l_discount <= 0.070000)) AND (F0_0.l_quantity < 24))) F1_0 ON ((1 = 1)))),
temp_view_1 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS revenue, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_2) F0_0),
temp_view_0 AS (
SELECT /*+ materialize */ F0_0.revenue AS revenue, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (SELECT * FROM temp_view_1) F0_0)
SELECT F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (SELECT * FROM temp_view_0) F0_0;


