copy (
    SELECT
        top_level_tp_table_0.column_0::bigint
    FROM
        traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
) to 'log_top.csv';