
SELECT F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.o_orderpriority AS o_orderpriority
FROM (
SELECT F0_0.o_orderpriority AS o_orderpriority, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey
FROM (
SELECT F0_0.o_orderdate AS o_orderdate, F0_0.o_orderpriority AS o_orderpriority, F0_0.prov_orders_o__orderkey AS prov_orders_o__orderkey, F1_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F1_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, greatest(F0_0._setprov_dup_count, F1_0._setprov_dup_count) AS _setprov_dup_count
FROM ((
SELECT F0_0.o_orderkey AS o_orderkey, F0_0.o_orderdate AS o_orderdate, F0_0.o_orderpriority AS o_orderpriority, F0_0.o_orderkey AS prov_orders_o__orderkey, 1 AS _setprov_dup_count
FROM orders F0_0) F0_0 JOIN (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, row_number() OVER (PARTITION BY F0_0.l_orderkey ORDER BY F0_0.l_orderkey) AS _setprov_dup_count
FROM (
SELECT F0_0.l_orderkey AS l_orderkey, F0_0.l_commitdate AS l_commitdate, F0_0.l_receiptdate AS l_receiptdate, F0_0.l_linenumber AS prov_lineitem_l__linenumber, F0_0.l_orderkey AS prov_lineitem_l__orderkey
FROM lineitem F0_0) F0_0
WHERE (F0_0.l_commitdate < F0_0.l_receiptdate)) F1_0 ON ((F0_0.o_orderkey = F1_0.l_orderkey)))) F0_0
WHERE ((F0_0.o_orderdate >= '1993-07-01') AND (F0_0.o_orderdate < '1993-10-01'))
ORDER BY o_orderpriority ASC NULLS LAST) F0_0;


