-- using 1755703766 as a seed to the RNG


select
	o_orderpriority,
	count(*) as order_count
from
	orders
where
	o_orderdate >= date '1993-08-01'
	and o_orderdate < date '1993-08-01' + interval '3' month
	and exists (
		select
			*
		from
			lineitem
		where
			l_orderkey = o_orderkey
			and l_commitdate < l_receiptdate
			AND (l_orderkey, l_linenumber) in (%B%)
	)
	AND o_orderkey in (%A%)
group by
	o_orderpriority
order by
	o_orderpriority;
