-- using 1755708657 as a seed to the RNG
create view
	revenue0 (supplier_no, total_revenue) as
select
	l_suppkey,
	sum(l_extendedprice * (1 - l_discount))
from
	lineitem
where
	l_shipdate >= date '1993-04-01'
	and l_shipdate < date '1993-04-01' + interval '3' month
group by
	l_suppkey;