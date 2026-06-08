
SELECT F0_0.l_returnflag AS l_returnflag, F0_0.l_linestatus AS l_linestatus, sum(F0_0.l_quantity) OVER (PARTITION BY F0_0.l_returnflag, F0_0.l_linestatus) AS sum_qty, sum(F0_0.l_extendedprice) OVER (PARTITION BY F0_0.l_returnflag, F0_0.l_linestatus) AS sum_base_price, sum((F0_0.l_extendedprice * (1 - F0_0.l_discount))) OVER (PARTITION BY F0_0.l_returnflag, F0_0.l_linestatus) AS sum_disc_price, sum(((F0_0.l_extendedprice * (1 - F0_0.l_discount)) * (1 + F0_0.l_tax))) OVER (PARTITION BY F0_0.l_returnflag, F0_0.l_linestatus) AS sum_charge, avg(F0_0.l_quantity) OVER (PARTITION BY F0_0.l_returnflag, F0_0.l_linestatus) AS avg_qty, avg(F0_0.l_extendedprice) OVER (PARTITION BY F0_0.l_returnflag, F0_0.l_linestatus) AS avg_price, avg(F0_0.l_discount) OVER (PARTITION BY F0_0.l_returnflag, F0_0.l_linestatus) AS avg_disc, count(1) OVER (PARTITION BY F0_0.l_returnflag, F0_0.l_linestatus) AS count_order, F0_0.prov_lineitem_l__orderkey AS prov_lineitem_l__orderkey, F0_0.prov_lineitem_l__linenumber AS prov_lineitem_l__linenumber
FROM (
SELECT F0_0.l_quantity AS l_quantity, F0_0.l_extendedprice AS l_extendedprice, F0_0.l_discount AS l_discount, F0_0.l_tax AS l_tax, F0_0.l_returnflag AS l_returnflag, F0_0.l_linestatus AS l_linestatus, F0_0.l_shipdate AS l_shipdate, F0_0.l_orderkey AS prov_lineitem_l__orderkey, F0_0.l_linenumber AS prov_lineitem_l__linenumber
FROM lineitem F0_0) F0_0
WHERE (F0_0.l_shipdate <= '1998-09-02')
ORDER BY l_returnflag ASC NULLS LAST, l_linestatus ASC NULLS LAST;


