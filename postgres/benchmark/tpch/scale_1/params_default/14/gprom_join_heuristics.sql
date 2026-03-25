
SELECT F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_part_p__partkey AS prov_part_p__partkey
FROM (
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_shipdate AS l_shipdate, F1_0.p_partkey AS p_partkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_part_p__partkey AS prov_part_p__partkey
FROM ((
SELECT F0_0.l_partkey AS l_partkey, F0_0.l_shipdate AS l_shipdate, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F0_0 CROSS JOIN (
SELECT F0_0.p_partkey AS p_partkey, F0_0.p_partkey AS prov_part_p__partkey
FROM part F0_0) F1_0)) F0_0
WHERE (((F0_0.l_partkey = F0_0.p_partkey) AND (F0_0.l_shipdate >= '1995-09-01')) AND (F0_0.l_shipdate < '1995-10-01'));


