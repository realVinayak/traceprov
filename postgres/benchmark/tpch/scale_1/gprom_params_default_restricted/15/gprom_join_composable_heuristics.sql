
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
SELECT F0_0."GROUP_0" AS supplier_no, F0_0."AGGR_0" AS total_revenue, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT SUM((F0_0.l_extendedprice * ((1)::numeric - F0_0.l_discount))) AS "AGGR_0", F0_0.l_suppkey AS "GROUP_0"
FROM (
SELECT F0_0.l_suppkey AS l_suppkey, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_shipdate AS l_shipdate
FROM lineitem F0_0) F0_0
WHERE ((F0_0.l_shipdate >= ('1996-01-01')::date) AND (F0_0.l_shipdate < ('1996-04-01')::date))
GROUP BY F0_0.l_suppkey) F0_0 JOIN (
SELECT F0_0.l_suppkey AS "_P_SIDE_GROUP_0", F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.l_suppkey AS l_suppkey, F0_0.l_shipdate AS l_shipdate, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F0_0
WHERE ((F0_0.l_shipdate >= ('1996-01-01')::date) AND (F0_0.l_shipdate < ('1996-04-01')::date))) F1_0 ON ((F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0")))) F1_0)) F0_0 CROSS JOIN (
SELECT F0_0."AGGR_0" AS "max(total_revenue)", F1_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F1_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM ((
SELECT max(F0_0."AGGR_0") AS "AGGR_0"
FROM (
SELECT SUM((F0_0.l_extendedprice * ((1)::numeric - F0_0.l_discount))) AS "AGGR_0", F0_0.l_suppkey AS "GROUP_0"
FROM (
SELECT F0_0.l_suppkey AS l_suppkey, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_shipdate AS l_shipdate
FROM lineitem F0_0) F0_0
WHERE ((F0_0.l_shipdate >= ('1996-01-01')::date) AND (F0_0.l_shipdate < ('1996-04-01')::date))
GROUP BY F0_0.l_suppkey) F0_0) F0_0 LEFT OUTER JOIN (
SELECT F1_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F1_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM ((
SELECT F0_0.l_suppkey AS "GROUP_0"
FROM (
SELECT F0_0.l_suppkey AS l_suppkey, F0_0.l_shipdate AS l_shipdate
FROM lineitem F0_0) F0_0
WHERE ((F0_0.l_shipdate >= ('1996-01-01')::date) AND (F0_0.l_shipdate < ('1996-04-01')::date))
GROUP BY F0_0.l_suppkey) F0_0 JOIN (
SELECT F0_0.l_suppkey AS "_P_SIDE_GROUP_0", F0_0."prov_lineitem_1_l__orderkey" AS "prov_lineitem_1_l__orderkey", F0_0."prov_lineitem_1_l__linenumber" AS "prov_lineitem_1_l__linenumber"
FROM (
SELECT F0_0.l_suppkey AS l_suppkey, F0_0.l_shipdate AS l_shipdate, F0_0.l_orderkey AS "prov_lineitem_1_l__orderkey", F0_0.l_linenumber AS "prov_lineitem_1_l__linenumber"
FROM lineitem F0_0) F0_0
WHERE ((F0_0.l_shipdate >= ('1996-01-01')::date) AND (F0_0.l_shipdate < ('1996-04-01')::date))) F1_0 ON ((F0_0."GROUP_0" IS NOT DISTINCT FROM F1_0."_P_SIDE_GROUP_0")))) F1_0 ON ((1 = 1)))) F1_0)) F0_0
WHERE ((F0_0.s_suppkey = F0_0.supplier_no) AND (F0_0.total_revenue = F0_0."max(total_revenue)"))
ORDER BY s_suppkey ASC NULLS LAST) F0_0;


