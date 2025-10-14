-- using 1755703766 as a seed to the RNG


select
	100.00 * sum(case
		when p_type like 'PROMO%'
			then l_extendedprice * (1 - l_discount)
		else 0
	end) / sum(l_extendedprice * (1 - l_discount)) as promo_revenue
from
	lineitem,
	part
where
	l_partkey = p_partkey
	and l_shipdate >= date '1997-12-01'
	and l_shipdate < date '1997-12-01' + interval '1' month
    AND (l_orderkey, l_linenumber) in (select l_orderkey,l_linenumber from layer_0)
    AND p_partkey in (select p_partkey from layer_0);
