-- using 1755693633 as a seed to the RNG


select
	c_custkey,
	c_name,
	sum(l_extendedprice * (1 - l_discount)) as revenue,
	c_acctbal,
	n_name,
	c_address,
	c_phone,
	c_comment
from
	customer,
	orders,
	lineitem,
	nation
where
	c_custkey = o_custkey
	and l_orderkey = o_orderkey
	and o_orderdate >= date '1994-06-01'
	and o_orderdate < date '1994-06-01' + interval '3' month
	and l_returnflag = 'R'
	and c_nationkey = n_nationkey
    AND c_custkey in (%A%)
    AND o_orderkey in (%B%)
    AND (l_orderkey, l_linenumber) in (%C%)
    AND n_nationkey in (%D%)
group by
	c_custkey,
	c_name,
	c_acctbal,
	c_phone,
	n_name,
	c_address,
	c_comment
order by
	revenue desc
LIMIT 20;