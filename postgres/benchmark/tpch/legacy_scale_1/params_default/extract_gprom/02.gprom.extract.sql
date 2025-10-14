PROVENANCE OF (
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
        part USE PROVENANCE (p_partkey),
        supplier USE PROVENANCE (s_suppkey),
        nation USE PROVENANCE (n_nationkey),
        region USE PROVENANCE (r_regionkey),
        partsupp USE PROVENANCE(ps_partkey, ps_suppkey) ps_main
        join (
            select
                min(ps_supplycost) as min_ps_suppcost,
                ps_partkey
            from
                partsupp USE PROVENANCE (ps_partkey, ps_suppkey),
                supplier USE PROVENANCE (s_suppkey),
                nation USE PROVENANCE (n_nationkey),
                region USE PROVENANCE (r_regionkey)
            where
                s_suppkey = ps_suppkey
                and s_nationkey = n_nationkey
                and n_regionkey = r_regionkey
                and r_name = 'EUROPE'
            group by ps_partkey
        ) as subq
        on ps_main.ps_supplycost = min_ps_suppcost and subq.ps_partkey = ps_main.ps_partkey
    where
        p_partkey = ps_main.ps_partkey
        and s_suppkey = ps_suppkey
        and p_size = 15
        and p_type like '%BRASS'
        and s_nationkey = n_nationkey
        and n_regionkey = r_regionkey
        and r_name = 'EUROPE'
    order by
        s_acctbal desc,
        n_name,
        s_name,
        p_partkey
    LIMIT 100
);


-- MOD -- 
-- select
-- 	s_acctbal,
-- 	s_name,
-- 	n_name,
-- 	p_partkey,
-- 	p_mfgr,
-- 	s_address,
-- 	s_phone,
-- 	s_comment
-- from
-- 	part,
-- 	supplier,
-- 	nation,
-- 	region,
--     partsupp ps_main
--     join (
-- 		select
-- 			min(ps_supplycost) as min_ps_suppcost,
--             ps_partkey
-- 		from
-- 			partsupp,
-- 			supplier,
-- 			nation,
-- 			region
-- 		where
-- 			s_suppkey = ps_suppkey
-- 			and s_nationkey = n_nationkey
-- 			and n_regionkey = r_regionkey
-- 			and r_name = 'EUROPE'
--         group by ps_partkey
--     ) as subq
--     on ps_main.ps_supplycost = min_ps_suppcost and subq.ps_partkey = ps_main.ps_partkey
-- where
-- 	p_partkey = ps_main.ps_partkey
-- 	and s_suppkey = ps_suppkey
-- 	and p_size = 15
-- 	and p_type like '%BRASS'
-- 	and s_nationkey = n_nationkey
-- 	and n_regionkey = r_regionkey
-- 	and r_name = 'EUROPE'
-- order by
-- 	s_acctbal desc,
-- 	n_name,
-- 	s_name,
-- 	p_partkey
-- LIMIT 100;

-- select
-- 	s_acctbal,
-- 	s_name,
-- 	n_name,
-- 	p_partkey,
-- 	p_mfgr,
-- 	s_address,
-- 	s_phone,
-- 	s_comment
-- from
-- 	part,
-- 	supplier,
-- 	partsupp,
-- 	nation,
-- 	region
-- where
-- 	p_partkey = ps_partkey
-- 	and s_suppkey = ps_suppkey
-- 	and p_size = 15
-- 	and p_type like '%BRASS'
-- 	and s_nationkey = n_nationkey
-- 	and n_regionkey = r_regionkey
-- 	and r_name = 'EUROPE'
-- 	and ps_supplycost = (
-- 		select
-- 			min(ps_supplycost)
-- 		from
-- 			partsupp,
-- 			supplier,
-- 			nation,
-- 			region
-- 		where
-- 			p_partkey = ps_partkey
-- 			and s_suppkey = ps_suppkey
-- 			and s_nationkey = n_nationkey
-- 			and n_regionkey = r_regionkey
-- 			and r_name = 'EUROPE'
-- 	)
-- order by
-- 	s_acctbal desc,
-- 	n_name,
-- 	s_name,
-- 	p_partkey
-- LIMIT 100;
