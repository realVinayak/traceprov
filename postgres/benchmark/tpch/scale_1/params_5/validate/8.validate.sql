-- using 1755708657 as a seed to the RNG


select
	o_year,
	sum(case
		when nation = 'INDONESIA' then volume
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
			and r_name = 'ASIA'
			and s_nationkey = n2.n_nationkey
			and o_orderdate between date '1995-01-01' and date '1996-12-31'
			and p_type = 'STANDARD BURNISHED TIN'
            AND p_partkey in (%A%)
            AND s_suppkey in (%B%)
            AND (l_orderkey, l_linenumber) in (%C%)
            AND o_orderkey in (%D%)
            AND c_custkey in (%E%)
            AND n1.n_nationkey in (%F%)
            AND n2.n_nationkey in (%G%)
            AND r_regionkey in (%H%)
	) as all_nations
group by
	o_year
order by
	o_year;
