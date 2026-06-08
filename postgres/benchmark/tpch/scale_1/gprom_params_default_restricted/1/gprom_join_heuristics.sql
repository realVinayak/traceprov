
SELECT F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0."GROUP_0" AS l_returnflag, F0_0."GROUP_1" AS l_linestatus, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F0_0.l_returnflag AS "GROUP_0", F0_0.l_linestatus AS "GROUP_1"
FROM (
SELECT F0_0.l_returnflag AS l_returnflag, F0_0.l_linestatus AS l_linestatus, F0_0.l_shipdate AS l_shipdate
FROM lineitem F0_0) F0_0
WHERE (F0_0.l_shipdate <= '1998-09-02')
GROUP BY F0_0.l_returnflag, F0_0.l_linestatus) F0_0 JOIN (
SELECT F0_0.l_returnflag AS "_P_SIDE_GROUP_0", F0_0.l_linestatus AS "_P_SIDE_GROUP_1", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.l_returnflag AS l_returnflag, F0_0.l_linestatus AS l_linestatus, F0_0.l_shipdate AS l_shipdate, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F0_0
WHERE (F0_0.l_shipdate <= '1998-09-02')) F1_0 ON (((F0_0."GROUP_1" = F1_0."_P_SIDE_GROUP_1") AND (F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0"))))
ORDER BY l_returnflag ASC NULLS LAST, l_linestatus ASC NULLS LAST) F0_0;


