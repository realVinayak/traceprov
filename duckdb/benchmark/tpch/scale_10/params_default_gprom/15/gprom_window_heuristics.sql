
SELECT F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM (
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.supplier_no AS supplier_no, F0_0.total_revenue AS total_revenue, F1_0."max(total_revenue)" AS "max(total_revenue)", F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F1_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM ((
SELECT F0_0.s_suppkey AS s_suppkey, F1_0.supplier_no AS supplier_no, F1_0.total_revenue AS total_revenue, F0_0.prov_supplier_s__suppkey AS prov_supplier_s__suppkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F0_0.s_suppkey AS s_suppkey, F0_0.s_suppkey AS prov_supplier_s__suppkey
FROM supplier F0_0) F0_0 CROSS JOIN (
SELECT F0_0.l_suppkey AS supplier_no, SUM((F0_0.l_extendedprice * ((1)::NUMERIC - F0_0.l_discount))) OVER (PARTITION BY F0_0.l_suppkey) AS total_revenue, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.l_suppkey AS l_suppkey, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_shipdate AS l_shipdate, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F0_0
WHERE ((F0_0.l_shipdate >= ('1996-01-01')::date) AND (F0_0.l_shipdate < ('1996-04-01')::date))) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0."AGGR_0" AS "max(total_revenue)", F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM (
SELECT F0_0.total_revenue AS total_revenue, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber", F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count, max((CASE  WHEN (1 = F0_0._setprov_dup_count) THEN F0_0.total_revenue ELSE (NULL)::int8 END)) OVER () AS "AGGR_0", count(1) OVER () AS __dummy_cnt
FROM ((
SELECT F0_0."AGGR_0" AS total_revenue, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber", dense_rank() OVER ( ORDER BY F0_0."AGG_GB_ARG1") AS _result_tid, row_number() OVER (PARTITION BY F0_0."AGG_GB_ARG1" ORDER BY F0_0."AGG_GB_ARG1") AS _setprov_dup_count
FROM (
SELECT (F0_0.l_extendedprice * ((1)::NUMERIC - F0_0.l_discount)) AS "AGG_GB_ARG0", F0_0.l_suppkey AS "AGG_GB_ARG1", F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber", SUM((F0_0.l_extendedprice * ((1)::NUMERIC - F0_0.l_discount))) OVER (PARTITION BY F0_0.l_suppkey) AS "AGGR_0"
FROM (
SELECT F0_0.l_suppkey AS l_suppkey, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_shipdate AS l_shipdate, F0_0.l_orderkey AS "prov_lineitem_1_l__orderkey", F0_0.l_linenumber AS "prov_lineitem_1_l__linenumber"
FROM lineitem F0_0) F0_0
WHERE ((F0_0.l_shipdate >= ('1996-01-01')::date) AND (F0_0.l_shipdate < ('1996-04-01')::date))) F0_0 UNION ALL (SELECT NULL AS total_revenue, NULL AS "prov_lineitem_1_l__orderkey", NULL AS "prov_lineitem_1_l__linenumber", -1 AS _result_tid, NULL AS _setprov_dup_count))) F0_0) F0_0
WHERE ((F0_0.__dummy_cnt = 1) OR (F0_0._result_tid <> -1))) F1_0)) F0_0
WHERE ((F0_0.s_suppkey = F0_0.supplier_no) AND (F0_0.total_revenue = F0_0."max(total_revenue)"))
ORDER BY s_suppkey ASC NULLS LAST) F0_0;


