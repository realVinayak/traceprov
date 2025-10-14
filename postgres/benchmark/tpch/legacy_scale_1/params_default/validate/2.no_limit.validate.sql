-- using default substitutions


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
	and p_size = 15
	and p_type like '%BRASS'
	and s_nationkey = n_nationkey
	and n_regionkey = r_regionkey
	and r_name = 'EUROPE'
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
			and r_name = 'EUROPE'
            AND    (ps_suppkey, ps_partkey) in (%F%)
            AND    s_suppkey in (%G%)
            AND    n_nationkey in (%H%)
            AND    r_regionkey in (%I%)
	)
AND      p_partkey in (%A%)
AND      s_suppkey in (%B%)
AND      n_nationkey in (%C%)
AND      r_regionkey in (%D%)
AND      (ps_partkey, ps_suppkey) in (%E%)
order by
	s_acctbal desc,
	n_name,
	s_name,
	p_partkey;
