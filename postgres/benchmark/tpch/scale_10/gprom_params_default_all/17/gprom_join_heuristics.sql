
SELECT (F0_0."AGGR_0" / 7.000000) AS avg_yearly, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_part_p__partkey AS prov_part_p__partkey, F1_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F1_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM ((
SELECT sum(F0_0.l_extendedprice) AS "AGGR_0"
FROM (((
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_quantity AS l_quantity, F0_0.l_extendedprice AS l_extendedprice
FROM lineitem F0_0) F0_0 CROSS JOIN (
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_brand AS p_brand, F0_0.p_container AS p_container
FROM part F0_0) F1_0) CROSS JOIN (
SELECT (0.200000 * avg(F0_0.l_quantity)) AS "(0200000*avg(l_quantity))", F0_0.l_partkey AS "l_partkey_1"
FROM lineitem F0_0
GROUP BY F0_0.l_partkey) F2_0)
WHERE (((((F1_0.p_partkey = F0_0.l_partkey) AND (F1_0.p_brand = 'Brand#23')) AND (F1_0.p_container = 'MED BOX')) AND (F0_0.l_quantity < F2_0."(0200000*avg(l_quantity))")) AND (F1_0.p_partkey = F2_0."l_partkey_1"))) F0_0 LEFT OUTER JOIN (
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_quantity AS l_quantity, F0_0.p_partkey AS p_partkey, F0_0.p_brand AS p_brand, F0_0.p_container AS p_container, F0_0."(0200000*avg(l_quantity))" AS "(0200000*avg(l_quantity))", F0_0."l_partkey_1" AS "l_partkey_1", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM (
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_quantity AS l_quantity, F0_0.p_partkey AS p_partkey, F0_0.p_brand AS p_brand, F0_0.p_container AS p_container, F1_0."(0200000*avg(l_quantity))" AS "(0200000*avg(l_quantity))", F1_0."l_partkey_1" AS "l_partkey_1", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey, F1_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F1_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM ((
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_quantity AS l_quantity, F1_0.p_partkey AS p_partkey, F1_0.p_brand AS p_brand, F1_0.p_container AS p_container, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_part_p__partkey AS prov_part_p__partkey
FROM ((
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_quantity AS l_quantity, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F0_0 CROSS JOIN (
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_brand AS p_brand, F0_0.p_container AS p_container, F0_0.p_partkey AS prov_part_p__partkey
FROM part F0_0) F1_0)) F0_0 CROSS JOIN (
SELECT (0.200000 * F0_0."AGGR_0") AS "(0200000*avg(l_quantity))", F0_0."l_partkey_1" AS "l_partkey_1", F1_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F1_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM ((
SELECT avg(F0_0.l_quantity) AS "AGGR_0", F0_0.l_partkey AS "l_partkey_1"
FROM lineitem F0_0
GROUP BY F0_0.l_partkey) F0_0 JOIN (
SELECT F0_0.l_partkey AS "_P_SIDE_l_partkey_1", F0_0.l_orderkey AS "prov_lineitem_1_l__orderkey", F0_0.l_linenumber AS "prov_lineitem_1_l__linenumber"
FROM lineitem F0_0) F1_0 ON ((F0_0."l_partkey_1" = F1_0."_P_SIDE_l_partkey_1")))) F1_0)) F0_0
WHERE (((((F0_0.p_partkey = F0_0.l_partkey) AND (F0_0.p_brand = 'Brand#23')) AND (F0_0.p_container = 'MED BOX')) AND (F0_0.l_quantity < F0_0."(0200000*avg(l_quantity))")) AND (F0_0.p_partkey = F0_0."l_partkey_1"))) F1_0 ON ((1 = 1)));


