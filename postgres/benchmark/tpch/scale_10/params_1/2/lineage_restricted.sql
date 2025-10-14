-- using 1755709829 as a seed to the RNG


select
	s_acctbal,
	s_name,
	n_name,
	p_partkey,
	p_mfgr,
	s_address,
	s_phone,
	s_comment
from
	part,
	supplier,
	partsupp,
	nation,
	region
where
	p_partkey = ps_partkey
	and s_suppkey = ps_suppkey
	and p_size = 2
	and p_type like '%STEEL'
	and s_nationkey = n_nationkey
	and n_regionkey = r_regionkey
	and r_name = 'AMERICA'
	and ps_supplycost = (
		select
			min(ps_supplycost)
		from
			partsupp,
			supplier,
			nation,
			region
		where
			p_partkey = ps_partkey
			and s_suppkey = ps_suppkey
			and s_nationkey = n_nationkey
			and n_regionkey = r_regionkey
			and r_name = 'AMERICA'
            AND    (ps_suppkey, ps_partkey) in (select ps_suppkey,ps_partkey from layer_0)
            AND    s_suppkey in (select s_suppkey from layer_0)
            AND    n_nationkey in (select n_nationkey from layer_0)
            AND    r_regionkey in (select r_regionkey from layer_0)
	)
AND      p_partkey in (select p_partkey from layer_1)
AND      s_suppkey in (select s_suppkey from layer_1)
AND      n_nationkey in (select n_nationkey from layer_1)
AND      r_regionkey in (select r_regionkey from layer_1)
AND      (ps_partkey, ps_suppkey) in (select ps_partkey,ps_suppkey from layer_1)
order by
	s_acctbal desc,
	n_name,
	s_name,
	p_partkey
LIMIT 100;
