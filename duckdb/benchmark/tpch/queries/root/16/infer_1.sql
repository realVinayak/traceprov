(
    SELECT
        tp_table_3.column_0,
        tp_table_4.column_1,
        tp_table_4.column_2,
        tp_table_4.column_3
    FROM
        (
            SELECT
                top_level_tp_table_0.column_0::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
        ) as tp_table_3 (column_0)
        JOIN (
            SELECT
                intermediate_join_tp_table_1.column_0::bigint,
                intermediate_join_tp_table_1.column_1::bigint,
                intermediate_join_tp_table_1.column_2::bigint,
                intermediate_join_tp_table_1.column_3::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 1::bigint) AS intermediate_join_tp_table_1
        ) as tp_table_4 (column_0, column_1, column_2, column_3) ON (tp_table_3.column_0 = tp_table_4.column_0)
)
UNION ALL
(
    select
        tp_table_3.column_0,
        tp_table_4.column_1,
        tp_table_4.column_2,
        tp_table_4.column_3
    FROM
        (
            SELECT
                top_level_tp_table_0.column_0::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
        ) as tp_table_3 (column_0)
        JOIN (
            SELECT
                combined_entry.column_0::bigint,
                combined_entry.column_1::bigint,
                combined_entry.column_2::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 5::bigint) AS combined_entry
        ) as tp_table_11 (column_0, column_1, column_2) ON (tp_table_3.column_0 = tp_table_11.column_0)
        JOIN (
            SELECT
                intermediate_join_tp_table_1.column_0::bigint,
                intermediate_join_tp_table_1.column_1::bigint,
                intermediate_join_tp_table_1.column_2::bigint,
                intermediate_join_tp_table_1.column_3::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 1::bigint) AS intermediate_join_tp_table_1
        ) as tp_table_4 (column_0, column_1, column_2, column_3) ON (tp_table_11.column_2 = tp_table_4.column_0)
)