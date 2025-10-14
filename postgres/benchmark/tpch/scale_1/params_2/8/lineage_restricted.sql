-- using 1755703766 as a seed to the RNG


select
	o_year,
	sum(case
		when nation = 'UNITED KINGDOM' then volume
		else 0
	end) / sum(volume) as mkt_share
from
	(
		select
			extract(year from o_orderdate) as o_year,
			l_extendedprice * (1 - l_discount) as volume,
			n2.n_name as nation
		from
			part,
			supplier,
			lineitem,
			orders,
			customer,
			nation n1,
			nation n2,
			region
		where
			p_partkey = l_partkey
			and s_suppkey = l_suppkey
			and l_orderkey = o_orderkey
			and o_custkey = c_custkey
			and c_nationkey = n1.n_nationkey
			and n1.n_regionkey = r_regionkey
			and r_name = 'EUROPE'
			and s_nationkey = n2.n_nationkey
			and o_orderdate between date '1995-01-01' and date '1996-12-31'
			and p_type = 'MEDIUM POLISHED STEEL'
            AND p_partkey in (select p_partkey from layer_0)
            AND s_suppkey in (select s_suppkey from layer_0)
            AND (l_orderkey, l_linenumber) in (select l_orderkey,l_linenumber from layer_0)
            AND o_orderkey in (select o_orderkey from layer_0)
            AND c_custkey in (select c_custkey from layer_0)
            AND n1.n_nationkey in (select n1_nationkey from layer_0)
            AND n2.n_nationkey in (select n2_nationkey from layer_0)
            AND r_regionkey in (select r_regionkey from layer_0)
	) as all_nations
group by
	o_year
order by
	o_year;
