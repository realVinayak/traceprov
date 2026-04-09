-- using default substitutions
-- $ID$
-- TPC-H/TPC-R Top Supplier Query (Q15)
-- Functional Query Definition
-- Approved February 1998
-- optimizations:
-- * skip read|writeFilter()
drop view if exists revenue0_2_row;
create view revenue0_2_row (tuid, l_orderkey) as
select _g.tuid as tuid,
	concat_agg(l_orderkey) as l_orderkey
from lineitem_2_row l,
	readAggregation(1, l.tuid) _g
group by _g.tuid;