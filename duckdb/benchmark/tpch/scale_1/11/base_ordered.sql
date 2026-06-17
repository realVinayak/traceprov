-- using default substitutions


select
	ps_partkey,
	sum(ps_supplycost * ps_availqty) as value
from
	q11_1_partsupp_ordered,
	q11_1_supplier_ordered,
	q11_1_nation_ordered
where
	ps_suppkey = s_suppkey
	and s_nationkey = n_nationkey
	and n_name = 'GERMANY'
group by
	ps_partkey having
		sum(ps_supplycost * ps_availqty) > (
			select
				sum(ps_supplycost * ps_availqty) * 0.0001000000
			from
				q11_2_partsupp_ordered,
				q11_2_supplier_ordered,
				q11_2_nation_ordered
			where
				ps_suppkey = s_suppkey
				and s_nationkey = n_nationkey
				and n_name = 'GERMANY'
		)
order by
	value desc;
