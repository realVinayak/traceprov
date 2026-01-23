copy ((
    SELECT
        tp_table_12.column_0,
        tp_table_13.column_1,
        tp_table_13.column_2
    FROM
        (
            SELECT
                top_level_tp_table_2.column_0::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_2
        ) as tp_table_12 (column_0)
        JOIN (
            SELECT
                intermediate_join_tp_table_3.column_0::bigint,
                intermediate_join_tp_table_3.column_1::bigint,
                intermediate_join_tp_table_3.column_2::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 1::bigint) AS intermediate_join_tp_table_3
        ) as tp_table_13 (column_0, column_1, column_2) ON (tp_table_12.column_0 = tp_table_13.column_0)
)
UNION ALL
(
    SELECT
        tp_table_15.column_0,
        tp_table_16.column_1,
        tp_table_16.column_2
    FROM
        (
            SELECT
                top_level_tp_table_4.column_0::bigint
            FROM
                traceprov_read_worker_layer (2::bigint, 2::bigint) AS top_level_tp_table_4
        ) as tp_table_15 (column_0)
        JOIN (
            SELECT
                intermediate_join_tp_table_5.column_0::bigint,
                intermediate_join_tp_table_5.column_1::bigint,
                intermediate_join_tp_table_5.column_2::bigint
            FROM
                traceprov_read_worker_layer (2::bigint, 1::bigint) AS intermediate_join_tp_table_5
        ) as tp_table_16 (column_0, column_1, column_2) ON (tp_table_15.column_0 = tp_table_16.column_0)
)
UNION ALL
(
    SELECT
        tp_table_18.column_0,
        tp_table_19.column_1,
        tp_table_19.column_2
    FROM
        (
            SELECT
                top_level_tp_table_6.column_0::bigint
            FROM
                traceprov_read_worker_layer (3::bigint, 2::bigint) AS top_level_tp_table_6
        ) as tp_table_18 (column_0)
        JOIN (
            SELECT
                intermediate_join_tp_table_7.column_0::bigint,
                intermediate_join_tp_table_7.column_1::bigint,
                intermediate_join_tp_table_7.column_2::bigint
            FROM
                traceprov_read_worker_layer (3::bigint, 1::bigint) AS intermediate_join_tp_table_7
        ) as tp_table_19 (column_0, column_1, column_2) ON (tp_table_18.column_0 = tp_table_19.column_0)
)
UNION ALL
(
    SELECT
        tp_table_21.column_0,
        tp_table_22.column_1,
        tp_table_22.column_2
    FROM
        (
            SELECT
                top_level_tp_table_8.column_0::bigint
            FROM
                traceprov_read_worker_layer (4::bigint, 2::bigint) AS top_level_tp_table_8
        ) as tp_table_21 (column_0)
        JOIN (
            SELECT
                intermediate_join_tp_table_9.column_0::bigint,
                intermediate_join_tp_table_9.column_1::bigint,
                intermediate_join_tp_table_9.column_2::bigint
            FROM
                traceprov_read_worker_layer (4::bigint, 1::bigint) AS intermediate_join_tp_table_9
        ) as tp_table_22 (column_0, column_1, column_2) ON (tp_table_21.column_0 = tp_table_22.column_0)
)) to '/home/realvinayak123/projects/traceprov/duckdb/playground/tpch/queries/root/18/infer_1_out.csv'