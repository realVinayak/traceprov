SELECT
    tp_table_0.l_shipmode,
    tp_table_0.high_line_count,
    tp_table_0.low_line_count,
    tp_table_0.mapped_agg,
    traceprov_log_entry_1 (2, tp_table_0.mapped_agg) AS tp_table_1
FROM
    (
        SELECT
            lineitem.l_shipmode,
            sum(
                CASE
                    WHEN (
                        (orders.o_orderpriority = '1-URGENT'::bpchar)
                        OR (orders.o_orderpriority = '2-HIGH'::bpchar)
                    ) THEN 1
                    ELSE 0
                END
            ) AS high_line_count,
            sum(
                CASE
                    WHEN (
                        (orders.o_orderpriority <> '1-URGENT'::bpchar)
                        AND (orders.o_orderpriority <> '2-HIGH'::bpchar)
                    ) THEN 1
                    ELSE 0
                END
            ) AS low_line_count,
            traceprov_agg_key_parallel_offset_3 (
                1,
                (orders.o_orderkey)::bigint,
                (lineitem.l_orderkey)::bigint,
                (lineitem.l_linenumber)::bigint
            ) AS mapped_agg
        FROM
            orders,
            lineitem
        WHERE
            (
                (orders.o_orderkey = lineitem.l_orderkey)
                AND (
                    lineitem.l_shipmode = ANY (ARRAY['MAIL'::bpchar, 'SHIP'::bpchar])
                )
                AND (lineitem.l_commitdate < lineitem.l_receiptdate)
                AND (lineitem.l_shipdate < lineitem.l_commitdate)
                AND (lineitem.l_receiptdate >= '1994-01-01'::date)
                AND (
                    lineitem.l_receiptdate < ('1994-01-01'::date + '1 year'::interval year)
                )
            )
        GROUP BY
            lineitem.l_shipmode
        ORDER BY
            lineitem.l_shipmode
    ) tp_table_0