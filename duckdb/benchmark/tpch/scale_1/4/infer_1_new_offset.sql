(
    SELECT
        tp_table_50.column_0,
        tp_table_50.column_1,
        tp_table_53.column_0,
        tp_table_53.column_1
    FROM
        (
            SELECT
                tp_table_51.column_0,
                tp_table_52.column_1
            FROM
                (
                    SELECT
                        top_level_tp_table_0.column_0::bigint
                    FROM
                        traceprov_read_worker_layer_offset (1::bigint, 3::bigint, __TP_OFFSET__::bigint) AS top_level_tp_table_0
                ) as tp_table_51 (column_0)
                JOIN (
                    SELECT
                        base_join_tp_table_1.column_0::bigint,
                        base_join_tp_table_1.column_1::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 2::bigint) AS base_join_tp_table_1
                ) as tp_table_52 (column_0, column_1) ON (tp_table_51.column_0 = tp_table_52.column_0)
        ) as tp_table_50 (column_0, column_1)
        JOIN (
            (
                SELECT
                    log_read_to_append_tp_table_6.column_0::bigint,
                    log_read_to_append_tp_table_6.column_1::bigint
                FROM
                    traceprov_read_worker_layer (1::bigint, 1::bigint) AS log_read_to_append_tp_table_6
            )
        ) as tp_table_53 (column_0, column_1) ON (tp_table_50.column_1 = tp_table_53.column_0)
)