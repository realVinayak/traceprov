copy (
    SELECT
        log_read_to_append_tp_table_6.column_0::bigint,
        log_read_to_append_tp_table_6.column_1::bigint,
        log_read_to_append_tp_table_6.column_2::bigint
    FROM
        traceprov_read_worker_layer (1::bigint, 1::bigint) AS log_read_to_append_tp_table_6
) to 'log_1.csv';