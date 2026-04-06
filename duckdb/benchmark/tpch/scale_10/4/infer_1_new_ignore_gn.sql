select
    tp_table_53.column_0,
    tp_table_53.column_1
FROM
    (
        SELECT
            base_join_tp_table_1.column_0::bigint
        FROM
            traceprov_read_worker_layer (1::bigint, 2::bigint) AS base_join_tp_table_1
    ) as tp_table_52 (column_0)
    JOIN (
        (
            SELECT
                log_read_to_append_tp_table_6.column_0::bigint,
                log_read_to_append_tp_table_6.column_1::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 1::bigint) AS log_read_to_append_tp_table_6
        )
    ) as tp_table_53 (column_0, column_1) ON (tp_table_52.column_0 = tp_table_53.column_0)