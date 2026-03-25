
SELECT F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0."GROUP_0" AS l_shipmode, F1_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F1_0.l_shipmode AS "GROUP_0"
FROM ((
SELECT F0_0.o_orderkey AS o_orderkey
FROM orders F0_0) F0_0 CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_shipdate AS l_shipdate, F0_0.l_commitdate AS l_commitdate, F0_0.l_receiptdate AS l_receiptdate, F0_0.l_shipmode AS l_shipmode
FROM lineitem F0_0) F1_0)
WHERE ((((((F0_0.o_orderkey = F1_0.l_orderkey) AND F1_0.l_shipmode IN ('MAIL', 'SHIP')) AND (F1_0.l_commitdate < F1_0.l_receiptdate)) AND (F1_0.l_shipdate < F1_0.l_commitdate)) AND (F1_0.l_receiptdate >= '1994-01-01')) AND (F1_0.l_receiptdate < '1995-01-01'))
GROUP BY F1_0.l_shipmode) F0_0 JOIN (
SELECT F0_0.l_shipmode AS "_P_SIDE_GROUP_0", F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.o_orderkey AS o_orderkey, F1_0.l_orderkey AS l_orderkey, F1_0.l_shipdate AS l_shipdate, F1_0.l_commitdate AS l_commitdate, F1_0.l_receiptdate AS l_receiptdate, F1_0.l_shipmode AS l_shipmode, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM ((
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_orderkey AS prov_orders_o__orderkey
FROM orders F0_0) F0_0 CROSS JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_shipdate AS l_shipdate, F0_0.l_commitdate AS l_commitdate, F0_0.l_receiptdate AS l_receiptdate, F0_0.l_shipmode AS l_shipmode, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F1_0)) F0_0
WHERE ((((((F0_0.o_orderkey = F0_0.l_orderkey) AND F0_0.l_shipmode IN ('MAIL', 'SHIP')) AND (F0_0.l_commitdate < F0_0.l_receiptdate)) AND (F0_0.l_shipdate < F0_0.l_commitdate)) AND (F0_0.l_receiptdate >= '1994-01-01')) AND (F0_0.l_receiptdate < '1995-01-01'))) F1_0 ON ((F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")))
ORDER BY l_shipmode ASC NULLS LAST) F0_0;


