SELECT
    tp_table_0.n_name,
    tp_table_0.revenue,
    tp_table_0.mapped_agg,
    traceprov_log_entry_1 (2, tp_table_0.mapped_agg) AS tp_table_1
FROM
    (
        SELECT
            nation.n_name,
            sum(
                (
                    lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
                )
            ) AS revenue,
            traceprov_agg_key_parallel_offset_7 (
                1,
                (customer.c_custkey)::bigint,
                (orders.o_orderkey)::bigint,
                (lineitem.l_orderkey)::bigint,
                (lineitem.l_linenumber)::bigint,
                (supplier.s_suppkey)::bigint,
                (nation.n_nationkey)::bigint,
                (region.r_regionkey)::bigint
            ) AS mapped_agg
        FROM
            customer,
            orders,
            lineitem,
            supplier,
            nation,
            region
        WHERE
            (
                (customer.c_custkey = orders.o_custkey)
                AND (lineitem.l_orderkey = orders.o_orderkey)
                AND (lineitem.l_suppkey = supplier.s_suppkey)
                AND (customer.c_nationkey = supplier.s_nationkey)
                AND (supplier.s_nationkey = nation.n_nationkey)
                AND (nation.n_regionkey = region.r_regionkey)
                AND (region.r_name = 'ASIA'::bpchar)
                AND (orders.o_orderdate >= '1994-01-01'::date)
                AND (
                    orders.o_orderdate < ('1994-01-01'::date + '1 year'::interval year)
                )
            )
        GROUP BY
            nation.n_name
        ORDER BY
            (
                sum(
                    (
                        lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
                    )
                )
            ) DESC
    ) tp_table_0