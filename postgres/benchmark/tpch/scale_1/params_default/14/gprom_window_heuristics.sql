
SELECT F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey
FROM (
SELECT F0_0."AGG_GB_ARG0" AS "AGG_GB_ARG0", F0_0."AGG_GB_ARG1" AS "AGG_GB_ARG1", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0._result_tid AS _result_tid, count(1) OVER () AS __dummy_cnt
FROM ((
SELECT (CASE  WHEN (F0_0.p_type LIKE 'PROMO%') THEN (F0_0.l_extendedprice * (1 - F0_0.l_discount)) ELSE 0 END) AS "AGG_GB_ARG0", (F0_0.l_extendedprice * (1 - F0_0.l_discount)) AS "AGG_GB_ARG1", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0._result_tid AS _result_tid
FROM (
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_shipdate AS l_shipdate, F1_0.p_partkey AS p_partkey, F1_0.p_type AS p_type, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_part_p__partkey AS prov_part_p__partkey, _mergerowid(F0_0._result_tid, F1_0._result_tid) AS _result_tid
FROM ((
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_shipdate AS l_shipdate, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber, _tid2int8(F0_0.ctid) AS _result_tid
FROM lineitem F0_0) F0_0 CROSS JOIN (
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_type AS p_type, F0_0.p_partkey AS prov_part_p__partkey, (F0_0.p_partkey)::int8 AS _result_tid
FROM part F0_0) F1_0)) F0_0
WHERE (((F0_0.l_partkey = F0_0.p_partkey) AND (F0_0.l_shipdate >= '1995-09-01')) AND (F0_0.l_shipdate < '1995-10-01')) UNION ALL (SELECT NULL AS "AGG_GB_ARG0", NULL AS "AGG_GB_ARG1", NULL AS prov_lineitem_l__orderkey, NULL AS prov_lineitem_l__linenumber, NULL AS prov_part_p__partkey, -1 AS _result_tid))) F0_0) F0_0
WHERE ((F0_0.__dummy_cnt = 1) OR (F0_0._result_tid <> -1));


