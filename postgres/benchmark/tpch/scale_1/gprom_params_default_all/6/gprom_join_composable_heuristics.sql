
SELECT F0_0."AGGR_0" AS revenue, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT sum((F0_0.l_extendedprice * F0_0.l_discount)) AS "AGGR_0"
FROM (
SELECT F0_0.l_quantity AS l_quantity, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_shipdate AS l_shipdate
FROM lineitem F0_0) F0_0
WHERE (((((F0_0.l_shipdate >= '1994-01-01') AND (F0_0.l_shipdate < '1995-01-01')) AND (F0_0.l_discount >= 0.050000)) AND (F0_0.l_discount <= 0.070000)) AND (F0_0.l_quantity < 24))) F0_0 LEFT OUTER JOIN (
SELECT F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.l_quantity AS l_quantity, F0_0.l_discount AS l_discount, F0_0.l_shipdate AS l_shipdate, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F0_0
WHERE (((((F0_0.l_shipdate >= '1994-01-01') AND (F0_0.l_shipdate < '1995-01-01')) AND (F0_0.l_discount >= 0.050000)) AND (F0_0.l_discount <= 0.070000)) AND (F0_0.l_quantity < 24))) F1_0 ON ((1 = 1)));


