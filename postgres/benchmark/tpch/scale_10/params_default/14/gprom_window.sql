WITH temp_view_2 AS (
SELECT /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0", F0_0."AGGR_1" AS "AGGR_1", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey, 1 AS _result_tid, ROW_NUMBER() OVER () AS _setprov_dup_count
FROM (
SELECT F0_0."AGG_GB_ARG0" AS "AGG_GB_ARG0", F0_0."AGG_GB_ARG1" AS "AGG_GB_ARG1", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count, sum(F0_0."AGG_GB_ARG0") OVER () AS "AGGR_0", sum(F0_0."AGG_GB_ARG1") OVER () AS "AGGR_1", count(1) OVER () AS __dummy_cnt
FROM ((
SELECT (CASE  WHEN (F0_0.p_type LIKE 'PROMO%') THEN (F0_0.l_extendedprice * (1 - F0_0.l_discount)) ELSE 0 END) AS "AGG_GB_ARG0", (F0_0.l_extendedprice * (1 - F0_0.l_discount)) AS "AGG_GB_ARG1", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_linenumber AS l_linenumber, F0_0.l_quantity AS l_quantity, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_tax AS l_tax, F0_0.l_returnflag AS l_returnflag, F0_0.l_linestatus AS l_linestatus, F0_0.l_shipdate AS l_shipdate, F0_0.l_commitdate AS l_commitdate, F0_0.l_receiptdate AS l_receiptdate, F0_0.l_shipinstruct AS l_shipinstruct, F0_0.l_shipmode AS l_shipmode, F0_0.l_comment AS l_comment, F1_0.p_partkey AS p_partkey, F1_0.p_name AS p_name, F1_0.p_mfgr AS p_mfgr, F1_0.p_brand AS p_brand, F1_0.p_type AS p_type, F1_0.p_size AS p_size, F1_0.p_container AS p_container, F1_0.p_retailprice AS p_retailprice, F1_0.p_comment AS p_comment, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_part_p__partkey AS prov_part_p__partkey, _mergerowid(F0_0._result_tid, F1_0._result_tid) AS _result_tid, greatest(F0_0._setprov_dup_count, F1_0._setprov_dup_count) AS _setprov_dup_count
FROM ((
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_partkey AS l_partkey, F0_0.l_suppkey AS l_suppkey, F0_0.l_linenumber AS l_linenumber, F0_0.l_quantity AS l_quantity, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_tax AS l_tax, F0_0.l_returnflag AS l_returnflag, F0_0.l_linestatus AS l_linestatus, F0_0.l_shipdate AS l_shipdate, F0_0.l_commitdate AS l_commitdate, F0_0.l_receiptdate AS l_receiptdate, F0_0.l_shipinstruct AS l_shipinstruct, F0_0.l_shipmode AS l_shipmode, F0_0.l_comment AS l_comment, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber, _tid2int8(F0_0.ctid) AS _result_tid, 1 AS _setprov_dup_count
FROM lineitem F0_0) F0_0 CROSS JOIN (
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_name AS p_name, F0_0.p_mfgr AS p_mfgr, F0_0.p_brand AS p_brand, F0_0.p_type AS p_type, F0_0.p_size AS p_size, F0_0.p_container AS p_container, F0_0.p_retailprice AS p_retailprice, F0_0.p_comment AS p_comment, F0_0.p_partkey AS prov_part_p__partkey, (F0_0.p_partkey)::int8 AS _result_tid, 1 AS _setprov_dup_count
FROM part F0_0) F1_0)) F0_0
WHERE (((F0_0.l_partkey = F0_0.p_partkey) AND (F0_0.l_shipdate >= '1995-09-01')) AND (F0_0.l_shipdate < '1995-10-01')) UNION ALL (SELECT NULL AS "AGG_GB_ARG0", NULL AS "AGG_GB_ARG1", NULL AS prov_lineitem_l__orderkey, NULL AS prov_lineitem_l__linenumber, NULL AS prov_part_p__partkey, -1 AS _result_tid, NULL AS _setprov_dup_count))) F0_0) F0_0
WHERE ((F0_0.__dummy_cnt = 1) OR (F0_0._result_tid <> -1))),
temp_view_1 AS (
SELECT /*+ materialize */ ((100.000000 * F0_0."AGGR_0") / F0_0."AGGR_1") AS promo_revenue, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (SELECT * FROM temp_view_2) F0_0),
temp_view_0 AS (
SELECT /*+ materialize */ F0_0.promo_revenue AS promo_revenue, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey
FROM (SELECT * FROM temp_view_1) F0_0)
SELECT F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey
FROM (SELECT * FROM temp_view_0) F0_0;


