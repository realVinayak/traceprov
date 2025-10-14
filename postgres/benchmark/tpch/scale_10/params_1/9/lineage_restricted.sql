-- using 1755709829 as a seed to the RNG


select
	nation,
	o_year,
	sum(amount) as sum_profit
from
	(
		select
			n_name as nation,
			extract(year from o_orderdate) as o_year,
			l_extendedprice * (1 - l_discount) - ps_supplycost * l_quantity as amount
		from
			part,
			supplier,
			lineitem,
			partsupp,
			orders,
			nation
		where
			s_suppkey = l_suppkey
			and ps_suppkey = l_suppkey
			and ps_partkey = l_partkey
			and p_partkey = l_partkey
			and o_orderkey = l_orderkey
			and s_nationkey = n_nationkey
			and p_name like '%lime%'
            AND p_partkey in (select p_partkey from layer_0)
            AND s_suppkey in (select s_suppkey from layer_0)
            AND (l_orderkey, l_linenumber) in (select l_orderkey,l_linenumber from layer_0)
            AND (ps_partkey, ps_suppkey) in (select ps_partkey,ps_suppkey from layer_0)
            AND o_orderkey in (select o_orderkey from layer_0)
            AND n_nationkey in (select n_nationkey from layer_0)
	) as profit
group by
	nation,
	o_year
order by
	nation,
	o_year desc;
