-- using 1755709841 as a seed to the RNG


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
	and n_name = 'ALGERIA'
    AND (ps_suppkey, ps_partkey) in (%A%)
    AND s_suppkey in (%B%)
    AND n_nationkey in (%C%)
group by
	ps_partkey having
		sum(ps_supplycost * ps_availqty) > (
			select
				sum(ps_supplycost * ps_availqty) * 0.0000100000
			from
				partsupp,
				supplier,
				nation
			where
				ps_suppkey = s_suppkey
				and s_nationkey = n_nationkey
				and n_name = 'ALGERIA'
                and (ps_suppkey, ps_partkey) in (%D%)
                and s_suppkey in (%E%)
                and n_nationkey in (%F%)
		)
order by
	value desc;
