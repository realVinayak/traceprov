-- using default substitutions
select
    *,
    traceprov_log_entry_1 (4, mapped_agg)
FROM
    (
        select
            ps_partkey,
            sum(ps_supplycost * ps_availqty) as value,
            traceprov_agg_key_parallel_offset_3 (3, partsupp.rowid, supplier.rowid, nation.rowid) as mapped_agg
        from
            partsupp,
            supplier,
            nation
        where
            ps_suppkey = s_suppkey
            and s_nationkey = n_nationkey
            and n_name = 'GERMANY'
        group by
            ps_partkey
        having
            sum(ps_supplycost * ps_availqty) > (
                select
                    summed
                from
                    (
                        select
                            sum(ps_supplycost * ps_availqty) * 0.0001000000 as summed,
                            traceprov_log_entry_volatile_1 (
                                2,
                                traceprov_agg_key_parallel_offset_3 (1, partsupp.rowid, supplier.rowid, nation.rowid)
                            )
                        from
                            partsupp,
                            supplier,
                            nation
                        where
                            ps_suppkey = s_suppkey
                            and s_nationkey = n_nationkey
                            and n_name = 'GERMANY'
                    ) f
            )
        order by
            value desc
    ) F;