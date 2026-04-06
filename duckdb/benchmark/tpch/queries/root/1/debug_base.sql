COPY (
    SELECT
        base_join_tp_table_1.column_0::bigint,
        base_join_tp_table_1.column_1::bigint,
        base_join_tp_table_1.column_2::bigint
    FROM
        traceprov_read_worker_layer (1::bigint, 1::bigint) AS base_join_tp_table_1
) to 'q1_base_layer1.csv';