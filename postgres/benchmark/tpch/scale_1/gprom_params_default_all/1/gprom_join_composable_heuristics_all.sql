
SELECT F0_0."GROUP_0" AS l_returnflag, F0_0."GROUP_1" AS l_linestatus, F0_0."AGGR_0" AS sum_qty, F0_0."AGGR_1" AS sum_base_price, F0_0."AGGR_2" AS sum_disc_price, F0_0."AGGR_3" AS sum_charge, F0_0."AGGR_4" AS avg_qty, F0_0."AGGR_5" AS avg_price, F0_0."AGGR_6" AS avg_disc, F0_0."AGGR_7" AS count_order, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT sum(F0_0.l_quantity) AS "AGGR_0", sum(F0_0.l_extendedprice) AS "AGGR_1", sum((F0_0.l_extendedprice * (1 - F0_0.l_discount))) AS "AGGR_2", sum(((F0_0.l_extendedprice * (1 - F0_0.l_discount)) * (1 + F0_0.l_tax))) AS "AGGR_3", avg(F0_0.l_quantity) AS "AGGR_4", avg(F0_0.l_extendedprice) AS "AGGR_5", avg(F0_0.l_discount) AS "AGGR_6", count(1) AS "AGGR_7", F0_0.l_returnflag AS "GROUP_0", F0_0.l_linestatus AS "GROUP_1"
FROM (
SELECT F0_0.l_quantity AS l_quantity, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_tax AS l_tax, F0_0.l_returnflag AS l_returnflag, F0_0.l_linestatus AS l_linestatus, F0_0.l_shipdate AS l_shipdate
FROM lineitem F0_0) F0_0
WHERE (F0_0.l_shipdate <= '1998-09-02')
GROUP BY F0_0.l_returnflag, F0_0.l_linestatus) F0_0 JOIN (
SELECT F0_0.l_returnflag AS "_P_SIDE_GROUP_0", F0_0.l_linestatus AS "_P_SIDE_GROUP_1", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.l_returnflag AS l_returnflag, F0_0.l_linestatus AS l_linestatus, F0_0.l_shipdate AS l_shipdate, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F0_0
WHERE (F0_0.l_shipdate <= '1998-09-02')) F1_0 ON (((F0_0."GROUP_1" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_1") AND (F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0"))))
ORDER BY l_returnflag ASC NULLS LAST, l_linestatus ASC NULLS LAST;


