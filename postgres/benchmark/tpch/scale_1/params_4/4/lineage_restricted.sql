-- using 1755708649 as a seed to the RNG


select
	o_orderpriority,
	count(*) as order_count
from
	orders
where
	o_orderdate >= date '1996-01-01'
	and o_orderdate < date '1996-01-01' + interval '3' month
	and exists (
		select
			*
		from
			lineitem
		where
			l_orderkey = o_orderkey
			and l_commitdate < l_receiptdate
			AND (l_orderkey, l_linenumber) in (select l_orderkey,l_linenumber from layer_1)
	)
	AND o_orderkey in (select o_orderkey from layer_0)
group by
	o_orderpriority
order by
	o_orderpriority;
