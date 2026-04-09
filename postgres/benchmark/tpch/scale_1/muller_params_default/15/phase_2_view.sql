-- using default substitutions
-- $ID$
-- TPC-H/TPC-R Top Supplier Query (Q15)
-- Functional Query Definition
-- Approved February 1998
-- optimizations:
-- * skip read|writeFilter()
drop view if exists revenue0_2;
create view revenue0_2 (tuid, supplier_no, total_revenue) as
select _g.tuid as tuid,
	concat_agg(l_suppkey) as l_suppkey,
	concat_agg(l_extendedprice | l_discount) as sum
from lineitem_2 l,
	readAggregation(1, l.tuid) _g
group by _g.tuid;