-- using default substitutions
select
    *,
    traceprov_log_entry_2 (5, rowid_1, rowid_2)
FROM
    (
        select
            s_name,
            s_address,
            supplier.rowid as rowid_1,
            nation.rowid as rowid_2
        from
            supplier,
            nation
        where
            s_suppkey in (
                select
                    ps_suppkey
                from
                    (
                        select
                            ps_suppkey,
                            traceprov_log_entry_volatile_1 (4, partsupp.rowid)
                        from
                            partsupp
                        where
                            ps_partkey in (
                                select
                                    p_partkey
                                from
                                    (
                                        select
                                            p_partkey,
                                            traceprov_log_entry_volatile_1 (1, part.rowid)
                                        from
                                            part
                                        where
                                            p_name like 'forest%'
                                    ) f
                            )
                            and ps_availqty > (
                                select
                                    summed
                                from
                                    (
                                        select
                                            0.5 * sum(l_quantity) as summed,
                                            traceprov_log_entry_volatile_2 (
                                                3,
                                                partsupp.rowid,
                                                traceprov_agg_key_parallel_offset_ignore_gn_1 (2, lineitem.rowid)
                                            )
                                        from
                                            lineitem
                                        where
                                            l_partkey = ps_partkey
                                            and l_suppkey = ps_suppkey
                                            and l_shipdate >= date '1994-01-01'
                                            and l_shipdate < date '1994-01-01' + interval '1' year
                                    )
                            )
                    ) f
            )
            and s_nationkey = n_nationkey
            and n_name = 'CANADA'
        order by
            s_name
    ) F;