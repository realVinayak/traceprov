SELECT
    tp_table_0.revenue,
    tp_table_0.mapped_agg,
    traceprov_log_entry_1 (2, tp_table_0.mapped_agg) AS tp_table_1
FROM
    (
        SELECT
            sum((lineitem.l_extendedprice * lineitem.l_discount)) AS revenue,
            traceprov_agg_key_parallel_offset_2 (
                1,
                (lineitem.l_orderkey)::bigint,
                (lineitem.l_linenumber)::bigint
            ) AS mapped_agg
        FROM
            lineitem
        WHERE
            (
                (lineitem.l_shipdate >= '1994-01-01'::date)
                AND (
                    lineitem.l_shipdate < ('1994-01-01'::date + '1 year'::interval year)
                )
                AND (
                    (lineitem.l_discount >= (0.06 - 0.01))
                    AND (lineitem.l_discount <= (0.06 + 0.01))
                )
                AND (lineitem.l_quantity < (24)::numeric)
            )
    ) tp_table_0