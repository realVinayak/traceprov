-- using default substitutions
select
    nation,
    o_year,
    sum(amount) as sum_profit,
    traceprov_log_entry_1 (
        2,
        traceprov_agg_key_parallel_offset_6 (
            1,
            (profit.tp_p_partkey)::bigint,
            (profit.tp_s_suppkey)::bigint,
            (profit.tp_l_orderkey)::bigint,
            (profit.tp_ps_partkey)::bigint,
            (profit.tp_o_orderkey)::bigint,
            (profit.tp_n_nationkey)::bigint
        )
    ) AS mapped_agg
from
    (
        select
            n_name as nation,
            extract(
                year
                from
                    o_orderdate
            ) as o_year,
            l_extendedprice * (1 - l_discount) - ps_supplycost * l_quantity as amount,
            part.rowid AS tp_p_partkey,
            supplier.rowid AS tp_s_suppkey,
            lineitem.rowid AS tp_l_orderkey,
            partsupp.rowid AS tp_ps_partkey,
            orders.rowid AS tp_o_orderkey,
            nation.rowid AS tp_n_nationkey
        from
            part,
            supplier,
            lineitem,
            partsupp,
            orders,
            nation
        where
            s_suppkey = l_suppkey
            and ps_suppkey = l_suppkey
            and ps_partkey = l_partkey
            and p_partkey = l_partkey
            and o_orderkey = l_orderkey
            and s_nationkey = n_nationkey
            and p_name like '%green%'
    ) as profit
group by
    nation,
    o_year
order by
    nation,
    o_year desc;