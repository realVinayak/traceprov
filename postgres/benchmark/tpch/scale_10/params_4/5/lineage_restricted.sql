-- using 1755709853 as a seed to the RNG


select
	n_name,
	sum(l_extendedprice * (1 - l_discount)) as revenue
from
	customer,
	orders,
	lineitem,
	supplier,
	nation,
	region
where
	c_custkey = o_custkey
	and l_orderkey = o_orderkey
	and l_suppkey = s_suppkey
	and c_nationkey = s_nationkey
	and s_nationkey = n_nationkey
	and n_regionkey = r_regionkey
	and r_name = 'MIDDLE EAST'
	and o_orderdate >= date '1994-01-01'
	and o_orderdate < date '1994-01-01' + interval '1' year
    AND c_custkey in (select c_custkey from layer_0)
    AND o_orderkey in (select o_orderkey from layer_0)
    AND (l_orderkey, l_linenumber) in (select l_orderkey,l_linenumber from layer_0)
    AND s_suppkey in (select s_suppkey from layer_0)
    AND n_nationkey in (select n_nationkey from layer_0)
    AND r_regionkey in (select r_regionkey from layer_0)
group by
	n_name
order by
	revenue desc;
