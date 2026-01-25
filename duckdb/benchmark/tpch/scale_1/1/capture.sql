SELECT
    tp_table_0.l_returnflag,
    tp_table_0.l_linestatus,
    tp_table_0.sum_qty,
    tp_table_0.sum_base_price,
    tp_table_0.sum_disc_price,
    tp_table_0.sum_charge,
    tp_table_0.avg_qty,
    tp_table_0.avg_price,
    tp_table_0.avg_disc,
    tp_table_0.count_order,
    tp_table_0.mapped_agg,
    traceprov_log_entry_1 (2, tp_table_0.mapped_agg) AS tp_table_1
FROM
    (
        SELECT
            lineitem.l_returnflag,
            lineitem.l_linestatus,
            sum(lineitem.l_quantity) AS sum_qty,
            sum(lineitem.l_extendedprice) AS sum_base_price,
            sum(
                (
                    lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
                )
            ) AS sum_disc_price,
            sum(
                (
                    (
                        lineitem.l_extendedprice * ((1)::numeric - lineitem.l_discount)
                    ) * ((1)::numeric + lineitem.l_tax)
                )
            ) AS sum_charge,
            avg(lineitem.l_quantity) AS avg_qty,
            avg(lineitem.l_extendedprice) AS avg_price,
            avg(lineitem.l_discount) AS avg_disc,
            count(*) AS count_order,
            traceprov_agg_key_parallel_offset_2 (
                1,
                (lineitem.l_orderkey)::bigint,
                (lineitem.l_linenumber)::bigint
            ) AS mapped_agg
        FROM
            lineitem
        WHERE
            (
                lineitem.l_shipdate <= ('1998-12-01'::date - '90 days'::interval day)
            )
        GROUP BY
            lineitem.l_returnflag,
            lineitem.l_linestatus
        ORDER BY
            lineitem.l_returnflag,
            lineitem.l_linestatus
    ) tp_table_0