-- using 1755708657 as a seed to the RNG


select
	sum(l_extendedprice) / 7.0 as avg_yearly
from
	lineitem,
	part
where
	p_partkey = l_partkey
	and p_brand = 'Brand#12'
	and p_container = 'SM BAG'
    AND p_partkey in (select p_partkey from layer_0)
    AND (l_orderkey, l_linenumber) in (select l_orderkey,l_linenumber from layer_0)
	and l_quantity < (
		select
			0.2 * avg(l_quantity)
		from
			lineitem
		where
			l_partkey = p_partkey
            and (l_orderkey, l_linenumber) in (select l_orderkey,l_linenumber from layer_1)
	);
