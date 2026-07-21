SELECT
    tp_table_0.c_custkey,
    tp_table_0.c_name,
    tp_table_0.revenue,
    tp_table_0.c_acctbal,
    tp_table_0.n_name,
    tp_table_0.c_address,
    tp_table_0.c_phone,
    tp_table_0.c_comment,
    tp_table_0.mapped_agg,
    traceprov_log_entry_1 (2, tp_table_0.mapped_agg) AS tp_table_1
FROM
    (
        SELECT
            customer.c_custkey,
            customer.c_name,
            sum(
                (
                    lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
                )
            ) AS revenue,
            customer.c_acctbal,
            nation.n_name,
            customer.c_address,
            customer.c_phone,
            customer.c_comment,
            traceprov_agg_key_parallel_offset_5 (
                1,
                (customer.c_custkey)::bigint,
                (orders.o_orderkey)::bigint,
                (lineitem.l_orderkey)::bigint,
                (lineitem.l_linenumber)::bigint,
                (nation.n_nationkey)::bigint
            ) AS mapped_agg
        FROM
            customer,
            orders,
            lineitem,
            nation
        WHERE
            (
                (customer.c_custkey = orders.o_custkey)
                AND (lineitem.l_orderkey = orders.o_orderkey)
                AND (orders.o_orderdate >= '1993-10-01'::date)
                AND (
                    orders.o_orderdate < ('1993-10-01'::date + '3 mons'::interval month)
                )
                AND (lineitem.l_returnflag = 'R'::bpchar)
                AND (customer.c_nationkey = nation.n_nationkey)
            )
        GROUP BY
            customer.c_custkey,
            customer.c_name,
            customer.c_acctbal,
            customer.c_phone,
            nation.n_name,
            customer.c_address,
            customer.c_comment
        ORDER BY
            (
                sum(
                    (
                        lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
                    )
                )
            ) DESC
        LIMIT
            20
    ) tp_table_0