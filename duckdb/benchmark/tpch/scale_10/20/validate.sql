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
                    p_partkey in (
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
                    and (l_orderkey, l_linenumber) in (
                        select
                            column_1_1,
                            column_2_1
                        from
                            LAYER_2
                    )
            )
            and (ps_partkey, ps_suppkey) in (
                select
                    "CAST(top_level_tp_table_2.column_0 AS BIGINT)",
                    "CAST(top_level_tp_table_2.column_1 AS BIGINT)"
                from
                    LAYER_4
            )
    )
    and (s_suppkey, n_nationkey) in (
        select
            "CAST(top_level_tp_table_0.column_0 AS BIGINT)",
            "CAST(top_level_tp_table_0.column_1 AS BIGINT)"
        from
            LAYER_5
    )
    and s_nationkey = n_nationkey
order by
    s_name;