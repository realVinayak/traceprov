-- using default substitutions


select
	s_name,
	s_address
from
	q20_5_supplier_ordered,
	q20_5_nation_ordered
where
	s_suppkey in (
		select
			ps_suppkey
		from
			q20_4_partsupp_ordered
		where
			ps_partkey in (
				select
					p_partkey
				from
					q20_1_part_ordered
				where
					p_name like 'forest%'
			)
			and ps_availqty > (
				select
					0.5 * sum(l_quantity)
				from
					q20_2_lineitem_ordered
				where
					l_partkey = ps_partkey
					and l_suppkey = ps_suppkey
					and l_shipdate >= date '1994-01-01'
					and l_shipdate < date '1994-01-01' + interval '1' year
			)
	)
	and s_nationkey = n_nationkey
	and n_name = 'CANADA'
order by
	s_name;
