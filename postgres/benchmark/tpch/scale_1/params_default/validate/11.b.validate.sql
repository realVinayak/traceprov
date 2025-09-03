-- using default substitutions


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
	AND (ps_partkey, ps_suppkey) in (SELECT group_outer.ps_partkey,group_outer.ps_suppkey FROM group_outer)
	AND s_suppkey in (SELECT group_outer.s_suppkey FROM group_outer)
	AND n_nationkey in (SELECT group_outer.n_nationkey FROM group_outer)
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
				AND (ps_suppkey, ps_partkey) in (%A%)
				AND s_suppkey in (%B%)
				AND n_nationkey in (%C%)
		)
order by
	value desc;
