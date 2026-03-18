-- using default substitutions
select s_acctbal,
    s_name,
    n_name,
    p_partkey,
    p_mfgr,
    s_address,
    s_phone,
    s_comment,
    traceprov_log_entry_5 (
        3,
        rowid_1::int,
        rowid_2::int,
        rowid_3::int,
        rowid_4::int,
        rowid_5::int
    )
FROM (
        select s_acctbal,
            s_name,
            n_name,
            p_partkey,
            p_mfgr,
            s_address,
            s_phone,
            s_comment,
            part.rowid as rowid_1,
            supplier.rowid as rowid_2,
            partsupp.rowid as rowid_3,
            nation.rowid as rowid_4,
            region.rowid as rowid_5
        from part,
            supplier,
            partsupp,
            nation,
            region
        where p_partkey = ps_partkey
            and s_suppkey = ps_suppkey
            and p_size = 15
            and p_type like '%BRASS'
            and s_nationkey = n_nationkey
            and n_regionkey = r_regionkey
            and r_name = 'EUROPE'
            and ps_supplycost = (
                select min_value
                from (
                        select min(ps_supplycost) as min_value,
                            traceprov_log_entry_volatile_2 (
                                2,
                                part.rowid::int,
                                traceprov_agg_key_parallel_offset_4 (
                                    1,
                                    partsupp.rowid::int,
                                    supplier.rowid::int,
                                    nation.rowid::int,
                                    region.rowid::int
                                )
                            )
                        from partsupp,
                            supplier,
                            nation,
                            region
                        where p_partkey = ps_partkey
                            and s_suppkey = ps_suppkey
                            and s_nationkey = n_nationkey
                            and n_regionkey = r_regionkey
                            and r_name = 'EUROPE'
                    ) f
            )
        order by s_acctbal desc,
            n_name,
            s_name,
            p_partkey
        LIMIT 100
    ) F;