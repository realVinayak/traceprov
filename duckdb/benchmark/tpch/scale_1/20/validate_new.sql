-- using default substitutions
select
    s_name,
    s_address
from
    supplier,
    nation
where
    s_suppkey in (
        select
            ps_suppkey
        from
            partsupp
        where
            ps_partkey in (
                select
                    p_partkey
                from
                    part
                where
                    part.rowid in (
                        select
                            "CAST(top_level_tp_table_1.column_0 AS BIGINT)"
                        from
                            LAYER_1
                    )
            )
            and ps_availqty > (
                select
                    0.5 * sum(l_quantity)
                from
                    lineitem
                where
                    l_partkey = ps_partkey
                    and l_suppkey = ps_suppkey
                    and lineitem.rowid in (
                        select
                            column_1_1
                        from
                            LAYER_2
                    )
            )
            and partsupp.rowid in (
                select
                    "CAST(top_level_tp_table_2.column_0 AS BIGINT)"
                from
                    LAYER_4
            )
    )
    and (supplier.rowid, nation.rowid) in (
        select
            "CAST(top_level_tp_table_0.column_0 AS BIGINT)",
            "CAST(top_level_tp_table_0.column_1 AS BIGINT)"
        from
            LAYER_5
    )
    and s_nationkey = n_nationkey
order by
    s_name;