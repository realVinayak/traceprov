
SELECT F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM (
SELECT F0_0.l_extendedprice AS l_extendedprice, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber", F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count, count(1) OVER () AS __dummy_cnt
FROM ((
SELECT F0_0.l_extendedprice AS l_extendedprice, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber", F0_0._result_tid AS _result_tid, F0_0._setprov_dup_count AS _setprov_dup_count
FROM (
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_quantity AS l_quantity, F0_0.l_extendedprice AS l_extendedprice, F0_0.p_partkey AS p_partkey, F0_0.p_brand AS p_brand, F0_0.p_container AS p_container, F1_0."(0200000*avg(l_quantity))" AS "(0200000*avg(l_quantity))", F1_0."l_partkey_1" AS "l_partkey_1", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F1_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F1_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber", _mergerowid(F0_0._result_tid, F1_0._result_tid) AS _result_tid, greatest(F0_0._setprov_dup_count, F1_0._setprov_dup_count) AS _setprov_dup_count
FROM ((
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_quantity AS l_quantity, F0_0.l_extendedprice AS l_extendedprice, F1_0.p_partkey AS p_partkey, F1_0.p_brand AS p_brand, F1_0.p_container AS p_container, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_part_p__partkey AS prov_part_p__partkey, _mergerowid(F0_0._result_tid, F1_0._result_tid) AS _result_tid, greatest(F0_0._setprov_dup_count, F1_0._setprov_dup_count) AS _setprov_dup_count
FROM ((
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_quantity AS l_quantity, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber, _tid2int8(F0_0.ctid) AS _result_tid, 1 AS _setprov_dup_count
FROM lineitem F0_0) F0_0 CROSS JOIN (
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_brand AS p_brand, F0_0.p_container AS p_container, F0_0.p_partkey AS prov_part_p__partkey, ()::int8 AS _result_tid, 1 AS _setprov_dup_count
FROM part F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT (0.200000 * F0_0."AGGR_0") AS "(0200000*avg(l_quantity))", F0_0.l_partkey AS "l_partkey_1", F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber", dense_rank() OVER ( ORDER BY F0_0.l_partkey) AS _result_tid, row_number() OVER (PARTITION BY F0_0.l_partkey ORDER BY F0_0.l_partkey) AS _setprov_dup_count
FROM (
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_quantity AS l_quantity, F0_0.l_orderkey AS "prov_lineitem_1_l__orderkey", F0_0.l_linenumber AS "prov_lineitem_1_l__linenumber", avg(F0_0.l_quantity) OVER (PARTITION BY F0_0.l_partkey) AS "AGGR_0"
FROM lineitem F0_0) F0_0) F1_0)) F0_0
WHERE (((((F0_0.p_partkey = F0_0.l_partkey) AND (F0_0.p_brand = 'Brand#23')) AND (F0_0.p_container = 'MED BOX')) AND (F0_0.l_quantity < F0_0."(0200000*avg(l_quantity))")) AND (F0_0.p_partkey = F0_0."l_partkey_1")) UNION ALL (SELECT NULL AS l_extendedprice, NULL AS prov_lineitem_l__orderkey, NULL AS prov_lineitem_l__linenumber, NULL AS prov_part_p__partkey, NULL AS "prov_lineitem_1_l__orderkey", NULL AS "prov_lineitem_1_l__linenumber", -1 AS _result_tid, NULL AS _setprov_dup_count))) F0_0) F0_0
WHERE ((F0_0.__dummy_cnt = 1) OR (F0_0._result_tid <> -1));


