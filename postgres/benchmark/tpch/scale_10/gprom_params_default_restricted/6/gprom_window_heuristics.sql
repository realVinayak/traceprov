
SELECT F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0."AGG_GB_ARG0" AS "AGG_GB_ARG0", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0._result_tid AS _result_tid, count(1) OVER () AS __dummy_cnt
FROM ((
SELECT (F0_0.l_extendedprice * F0_0.l_discount) AS "AGG_GB_ARG0", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0._result_tid AS _result_tid
FROM (
SELECT F0_0.l_quantity AS l_quantity, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_shipdate AS l_shipdate, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber, _tid2int8(F0_0.ctid) AS _result_tid
FROM lineitem F0_0) F0_0
WHERE (((((F0_0.l_shipdate >= '1994-01-01') AND (F0_0.l_shipdate < '1995-01-01')) AND (F0_0.l_discount >= 0.050000)) AND (F0_0.l_discount <= 0.070000)) AND (F0_0.l_quantity < 24)) UNION ALL (SELECT NULL AS "AGG_GB_ARG0", NULL AS prov_lineitem_l__orderkey, NULL AS prov_lineitem_l__linenumber, -1 AS _result_tid))) F0_0) F0_0
WHERE ((F0_0.__dummy_cnt = 1) OR (F0_0._result_tid <> -1));


