-- using default substitutions


select
	sum(l_extendedprice) / 7.0 as avg_yearly
from
	q17_3_lineitem_ordered,
	q17_3_part_ordered
where
	p_partkey = l_partkey
	and p_brand = 'Brand#23'
	and p_container = 'MED BOX'
	and l_quantity < (
		select
			0.2 * avg(l_quantity)
		from
			q17_1_part_ordered
		where
			l_partkey = p_partkey
	);
