SELECT
    log_read_to_append_tp_table_1.column_0::bigint,
    log_read_to_append_tp_table_1.column_1::bigint
FROM
    traceprov_read_worker_layer (1::bigint, 2::bigint) AS log_read_to_append_tp_table_1;