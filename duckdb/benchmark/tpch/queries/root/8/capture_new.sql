-- using default substitutions


select
	o_year,
	sum(case
		when nation = 'BRAZIL' then volume
		else 0
	end) / sum(volume) as mkt_share,
    traceprov_log_entry_1(
        2,
        traceprov_agg_key_parallel_offset_8 (
            1,
            (all_nations.tp_p_partkey)::bigint,
            (all_nations.tp_s_suppkey)::bigint,
            (all_nations.tp_l_orderkey)::bigint,
            (all_nations.tp_o_orderkey)::bigint,
            (all_nations.tp_c_custkey)::bigint,
            (all_nations.tp_n_nationkey)::bigint,
            (all_nations.tp_n_nationkey_1)::bigint,
            (all_nations.tp_r_regionkey)::bigint
        ) AS mapped_agg
    )
from
	(
		select
			extract(year from o_orderdate) as o_year,
			l_extendedprice * (1 - l_discount) as volume,
			n2.n_name as nation,
            part.rowid AS tp_p_partkey,
            supplier.rowid AS tp_s_suppkey,
            lineitem.rowid AS tp_l_orderkey,
            orders.rowid AS tp_o_orderkey,
            customer.rowid AS tp_c_custkey,
            n1.rowid AS tp_n_nationkey,
            n2.rowid AS tp_n_nationkey_1,
            region.rowid AS tp_r_regionkey
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
			and r_name = 'AMERICA'
			and s_nationkey = n2.n_nationkey
			and o_orderdate between date '1995-01-01' and date '1996-12-31'
			and p_type = 'ECONOMY ANODIZED STEEL'
	) as all_nations
group by
	o_year
order by
	o_year;
