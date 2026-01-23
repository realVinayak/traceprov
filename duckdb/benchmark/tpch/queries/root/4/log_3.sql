copy (
    SELECT
        top_level_tp_table_0.column_0::bigint
    FROM
        traceprov_read_worker_layer (1::bigint, 3::bigint) AS top_level_tp_table_0
) to 'log_3.csv';