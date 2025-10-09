-- using 1755703766 as a seed to the RNG


select
	sum(l_extendedprice) / 7.0 as avg_yearly
from
	lineitem,
	part
where
	p_partkey = l_partkey
	and p_brand = 'Brand#35'
	and p_container = 'SM PKG'
    AND p_partkey in (%A%)
    AND (l_orderkey, l_linenumber) in (%B%)
	and l_quantity < (
		select
			0.2 * avg(l_quantity)
		from
			lineitem
		where
			l_partkey = p_partkey
            and (l_orderkey, l_linenumber) in (%C%)
	);
