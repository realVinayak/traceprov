SELECT
    tp_table_0.promo_revenue,
    tp_table_0.mapped_agg,
    traceprov_log_entry_1 (2, tp_table_0.mapped_agg) AS tp_table_1
FROM
    (
        SELECT
            (
                (
                    100.00 * sum(
                        CASE
                            WHEN ((part.p_type)::text ~~ 'PROMO%'::text) THEN (
                                lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
                            )
                            ELSE (0)::numeric
                        END
                    )
                ) / sum(
                    (
                        lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
                    )
                )
            ) AS promo_revenue,
            traceprov_agg_key_parallel_offset_3 (
                1,
                (lineitem.l_orderkey)::bigint,
                (lineitem.l_linenumber)::bigint,
                (part.p_partkey)::bigint
            ) AS mapped_agg
        FROM
            lineitem,
            part
        WHERE
            (
                (lineitem.l_partkey = part.p_partkey)
                AND (lineitem.l_shipdate >= '1995-09-01'::date)
                AND (
                    lineitem.l_shipdate < ('1995-09-01'::date + '1 mon'::interval month)
                )
            )
    ) tp_table_0