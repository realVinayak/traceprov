-- using 1755693633 as a seed to the RNG


select
	ps_partkey,
	sum(ps_supplycost * ps_availqty) as value
from
	partsupp,
	supplier,
	nation
where
	ps_suppkey = s_suppkey
	and s_nationkey = n_nationkey
	and n_name = 'GERMANY'
    AND (ps_suppkey, ps_partkey) in (select ps_suppkey,ps_partkey from layer_0)
    AND s_suppkey in (select s_suppkey from layer_0)
    AND n_nationkey in (select n_nationkey from layer_0)
group by
	ps_partkey having
		sum(ps_supplycost * ps_availqty) > (
			select
				sum(ps_supplycost * ps_availqty) * 0.0001000000
			from
				partsupp,
				supplier,
				nation
			where
				ps_suppkey = s_suppkey
				and s_nationkey = n_nationkey
				and n_name = 'GERMANY'
                and (ps_suppkey, ps_partkey) in (select ps_suppkey,ps_partkey from layer_1)
                and s_suppkey in (select s_suppkey from layer_1)
                and n_nationkey in (select n_nationkey from layer_1)
		)
order by
	value desc;
