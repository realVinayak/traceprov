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
	and s_nationkey = n_nationkey
	and n_regionkey = r_regionkey
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
			and (partsupp.rowid) in (select opid_10_partsupp from LAYER_1)
	)
	and 
	-- (part.rowid, supplier.rowid, partsupp.rowid, nation.rowid, region.rowid)
	(part.rowid, supplier.rowid, partsupp.rowid, nation.rowid, region.rowid)
	in (select opid_20_part, opid_22_supplier, opid_19_partsupp, opid_24_nation, opid_25_region from LAYER_1)
order by
	s_acctbal desc,
	n_name,
	s_name,
	p_partkey
LIMIT 100;
