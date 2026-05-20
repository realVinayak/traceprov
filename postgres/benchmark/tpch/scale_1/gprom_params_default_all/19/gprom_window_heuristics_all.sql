
SELECT F0_0."AGGR_0" AS revenue, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey
FROM (
SELECT F0_0."AGG_GB_ARG0" AS "AGG_GB_ARG0", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0._result_tid AS _result_tid, sum(F0_0."AGG_GB_ARG0") OVER () AS "AGGR_0", count(1) OVER () AS __dummy_cnt
FROM ((
SELECT (F0_0.l_extendedprice * (1 - F0_0.l_discount)) AS "AGG_GB_ARG0", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0._result_tid AS _result_tid
FROM (
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_quantity AS l_quantity, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_shipinstruct AS l_shipinstruct, F0_0.l_shipmode AS l_shipmode, F1_0.p_partkey AS p_partkey, F1_0.p_brand AS p_brand, F1_0.p_size AS p_size, F1_0.p_container AS p_container, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_part_p__partkey AS prov_part_p__partkey, _mergerowid(F0_0._result_tid, F1_0._result_tid) AS _result_tid
FROM ((
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_quantity AS l_quantity, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_shipinstruct AS l_shipinstruct, F0_0.l_shipmode AS l_shipmode, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber, _tid2int8(F0_0.ctid) AS _result_tid
FROM lineitem F0_0) F0_0 CROSS JOIN (
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_brand AS p_brand, F0_0.p_size AS p_size, F0_0.p_container AS p_container, F0_0.p_partkey AS prov_part_p__partkey, (F0_0.p_partkey)::int8 AS _result_tid
FROM part F0_0) F1_0)) F0_0
WHERE (((((((((((F0_0.p_partkey = F0_0.l_partkey) AND (F0_0.p_brand = 'Brand#12')) AND F0_0.p_container IN ('SM CASE', 'SM BOX', 'SM PACK', 'SM PKG')) AND (F0_0.l_quantity >= 1)) AND (F0_0.l_quantity <= (1 + 10))) AND (F0_0.p_size >= 1)) AND (F0_0.p_size <= 5)) AND F0_0.l_shipmode IN ('AIR', 'AIR REG')) AND (F0_0.l_shipinstruct = 'DELIVER IN PERSON')) OR (((((((((F0_0.p_partkey = F0_0.l_partkey) AND (F0_0.p_brand = 'Brand#23')) AND F0_0.p_container IN ('MED BAG', 'MED BOX', 'MED PKG', 'MED PACK')) AND (F0_0.l_quantity >= 10)) AND (F0_0.l_quantity <= (10 + 10))) AND (F0_0.p_size >= 1)) AND (F0_0.p_size <= 10)) AND F0_0.l_shipmode IN ('AIR', 'AIR REG')) AND (F0_0.l_shipinstruct = 'DELIVER IN PERSON'))) OR (((((((((F0_0.p_partkey = F0_0.l_partkey) AND (F0_0.p_brand = 'Brand#34')) AND F0_0.p_container IN ('LG CASE', 'LG BOX', 'LG PACK', 'LG PKG')) AND (F0_0.l_quantity >= 20)) AND (F0_0.l_quantity <= (20 + 10))) AND (F0_0.p_size >= 1)) AND (F0_0.p_size <= 15)) AND F0_0.l_shipmode IN ('AIR', 'AIR REG')) AND (F0_0.l_shipinstruct = 'DELIVER IN PERSON'))) UNION ALL (SELECT NULL AS "AGG_GB_ARG0", NULL AS prov_lineitem_l__orderkey, NULL AS prov_lineitem_l__linenumber, NULL AS prov_part_p__partkey, -1 AS _result_tid))) F0_0) F0_0
WHERE ((F0_0.__dummy_cnt = 1) OR (F0_0._result_tid <> -1));


