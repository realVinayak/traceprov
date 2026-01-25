SELECT
    tp_table_0.nation,
    tp_table_0.o_year,
    tp_table_0.sum_profit,
    tp_table_0.mapped_agg,
    traceprov_log_entry_1 (2, tp_table_0.mapped_agg) AS tp_table_1
FROM
    (
        SELECT
            profit.nation,
            profit.o_year,
            sum(profit.amount) AS sum_profit,
            traceprov_agg_key_parallel_offset_8 (
                1,
                (profit.tp_p_partkey)::bigint,
                (profit.tp_s_suppkey)::bigint,
                (profit.tp_l_orderkey)::bigint,
                (profit.tp_l_linenumber)::bigint,
                (profit.tp_ps_partkey)::bigint,
                (profit.tp_ps_suppkey)::bigint,
                (profit.tp_o_orderkey)::bigint,
                (profit.tp_n_nationkey)::bigint
            ) AS mapped_agg
        FROM
            (
                SELECT
                    nation.n_name AS nation,
                    EXTRACT(
                        year
                        FROM
                            orders.o_orderdate
                    ) AS o_year,
                    (
                        (
                            lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
                        ) - (partsupp.ps_supplycost * lineitem.l_quantity)
                    ) AS amount,
                    part.p_partkey AS tp_p_partkey,
                    supplier.s_suppkey AS tp_s_suppkey,
                    lineitem.l_orderkey AS tp_l_orderkey,
                    lineitem.l_linenumber AS tp_l_linenumber,
                    partsupp.ps_partkey AS tp_ps_partkey,
                    partsupp.ps_suppkey AS tp_ps_suppkey,
                    orders.o_orderkey AS tp_o_orderkey,
                    nation.n_nationkey AS tp_n_nationkey
                FROM
                    part,
                    supplier,
                    lineitem,
                    partsupp,
                    orders,
                    nation
                WHERE
                    (
                        (supplier.s_suppkey = lineitem.l_suppkey)
                        AND (partsupp.ps_suppkey = lineitem.l_suppkey)
                        AND (partsupp.ps_partkey = lineitem.l_partkey)
                        AND (part.p_partkey = lineitem.l_partkey)
                        AND (orders.o_orderkey = lineitem.l_orderkey)
                        AND (supplier.s_nationkey = nation.n_nationkey)
                        AND ((part.p_name)::text ~~ '%green%'::text)
                    )
            ) profit
        GROUP BY
            profit.nation,
            profit.o_year
        ORDER BY
            profit.nation,
            profit.o_year DESC
    ) tp_table_0