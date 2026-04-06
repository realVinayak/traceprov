SELECT
    tp_table_6.s_name,
    tp_table_6.s_address,
    tp_table_6.tp_s_suppkey,
    tp_table_6.tp_n_nationkey,
    traceprov_log_entry_2 (
        5,
        (tp_table_6.tp_s_suppkey)::bigint,
        (tp_table_6.tp_n_nationkey)::bigint
    ) AS tp_table_7
FROM
    (
        SELECT
            supplier.s_name,
            supplier.s_address,
            supplier.s_suppkey AS tp_s_suppkey,
            nation.n_nationkey AS tp_n_nationkey
        FROM
            supplier,
            nation
        WHERE
            (
                (
                    supplier.s_suppkey IN (
                        SELECT
                            tp_table_5.ps_suppkey
                        FROM
                            (
                                SELECT
                                    partsupp.ps_suppkey,
                                    traceprov_log_entry_volatile_2 (
                                        4,
                                        (partsupp.ps_partkey)::bigint,
                                        (partsupp.ps_suppkey)::bigint
                                    ) AS tp_table_4
                                FROM
                                    partsupp
                                WHERE
                                    (
                                        (
                                            partsupp.ps_partkey IN (
                                                SELECT
                                                    tp_table_1.p_partkey
                                                FROM
                                                    (
                                                        SELECT
                                                            part.p_partkey,
                                                            traceprov_log_entry_volatile_1 (1, (part.p_partkey)::bigint) AS tp_table_0
                                                        FROM
                                                            part
                                                        WHERE
                                                            ((part.p_name)::text ~~ 'forest%'::text)
                                                    ) tp_table_1
                                            )
                                        )
                                        AND (
                                            (partsupp.ps_availqty)::numeric > (
                                                SELECT
                                                    tp_table_3."?column?"
                                                FROM
                                                    (
                                                        SELECT
                                                            (0.5 * sum(lineitem.l_quantity)) AS "?column?",
                                                            traceprov_log_entry_volatile_3 (
                                                                3,
                                                                (partsupp.ps_partkey)::bigint,
                                                                (partsupp.ps_suppkey)::bigint,
                                                                traceprov_agg_key_parallel_offset_2 (
                                                                    2,
                                                                    (lineitem.l_orderkey)::bigint,
                                                                    (lineitem.l_linenumber)::bigint
                                                                )
                                                            ) AS tp_table_2
                                                        FROM
                                                            lineitem
                                                        WHERE
                                                            (
                                                                (lineitem.l_partkey = partsupp.ps_partkey)
                                                                AND (lineitem.l_suppkey = partsupp.ps_suppkey)
                                                                AND (lineitem.l_shipdate >= '1994-01-01'::date)
                                                                AND (
                                                                    lineitem.l_shipdate < ('1994-01-01'::date + '1 year'::interval year)
                                                                )
                                                            )
                                                    ) tp_table_3
                                            )
                                        )
                                    )
                            ) tp_table_5
                    )
                )
                AND (supplier.s_nationkey = nation.n_nationkey)
                AND (nation.n_name = 'CANADA'::bpchar)
            )
        ORDER BY
            supplier.s_name
    ) tp_table_6