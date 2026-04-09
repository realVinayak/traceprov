-- using default substitutions
-- $ID$
-- TPC-H/TPC-R Top Supplier Query (Q15)
-- Functional Query Definition
-- Approved February 1998
-- optimizations:
-- * skip read|writeFilter()
drop view if exists revenue0_1;
create view revenue0_1 (tuid, supplier_no, total_revenue) as
select writeAggregation(1, array_agg(l.tuid)) as tuid,
	l_suppkey as l_suppkey,
	sum(l_extendedprice * (1 - l_discount)) as sum
from lineitem_1 l
where l_shipdate >= date '1996-01-01'
	and l_shipdate < date '1996-01-01' + interval '3' month
group by l_suppkey;