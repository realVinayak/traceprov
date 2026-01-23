copy ((
    SELECT
        tp_table_9.column_0,
        tp_table_12.column_1,
        tp_table_12.column_2
    FROM
        (
            SELECT
                tp_table_10.column_0,
                tp_table_11.column_1,
                tp_table_11.column_2
            FROM
                (
                    SELECT
                        top_level_tp_table_0.column_0::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
                ) as tp_table_10 (column_0)
                JOIN (
                    SELECT
                        combined_entry.column_0::bigint,
                        combined_entry.column_1::bigint,
                        combined_entry.column_2::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 5::bigint) AS combined_entry
                ) as tp_table_11 (column_0, column_1, column_2) ON (tp_table_10.column_0 = tp_table_11.column_0)
        ) as tp_table_9 (column_0, column_1, column_2)
        JOIN (
            SELECT
                base_join_tp_table_1.column_0::bigint,
                base_join_tp_table_1.column_1::bigint,
                base_join_tp_table_1.column_2::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 1::bigint) AS base_join_tp_table_1
        ) as tp_table_12 (column_0, column_1, column_2) ON (
            tp_table_9.column_2 = tp_table_12.column_0
            AND tp_table_9.column_1 = 1
        )
)
UNION ALL
(
    SELECT
        tp_table_14.column_0,
        tp_table_15.column_1,
        tp_table_15.column_2
    FROM
        (
            SELECT
                top_level_tp_table_0.column_0::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
        ) as tp_table_14 (column_0)
        JOIN (
            SELECT
                base_join_tp_table_1.column_0::bigint,
                base_join_tp_table_1.column_1::bigint,
                base_join_tp_table_1.column_2::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 1::bigint) AS base_join_tp_table_1
        ) as tp_table_15 (column_0, column_1, column_2) ON (tp_table_14.column_0 = tp_table_15.column_0)
)
UNION ALL
(
    SELECT
        tp_table_17.column_0,
        tp_table_20.column_1,
        tp_table_20.column_2
    FROM
        (
            SELECT
                tp_table_18.column_0,
                tp_table_19.column_1,
                tp_table_19.column_2
            FROM
                (
                    SELECT
                        top_level_tp_table_0.column_0::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
                ) as tp_table_18 (column_0)
                JOIN (
                    SELECT
                        combined_entry.column_0::bigint,
                        combined_entry.column_1::bigint,
                        combined_entry.column_2::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 5::bigint) AS combined_entry
                ) as tp_table_19 (column_0, column_1, column_2) ON (tp_table_18.column_0 = tp_table_19.column_0)
        ) as tp_table_17 (column_0, column_1, column_2)
        JOIN (
            SELECT
                base_join_tp_table_2.column_0::bigint,
                base_join_tp_table_2.column_1::bigint,
                base_join_tp_table_2.column_2::bigint
            FROM
                traceprov_read_worker_layer (2::bigint, 1::bigint) AS base_join_tp_table_2
        ) as tp_table_20 (column_0, column_1, column_2) ON (
            tp_table_17.column_2 = tp_table_20.column_0
            AND tp_table_17.column_1 = 2
        )
)
UNION ALL
(
    SELECT
        tp_table_22.column_0,
        tp_table_23.column_1,
        tp_table_23.column_2
    FROM
        (
            SELECT
                top_level_tp_table_0.column_0::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
        ) as tp_table_22 (column_0)
        JOIN (
            SELECT
                base_join_tp_table_2.column_0::bigint,
                base_join_tp_table_2.column_1::bigint,
                base_join_tp_table_2.column_2::bigint
            FROM
                traceprov_read_worker_layer (2::bigint, 1::bigint) AS base_join_tp_table_2
        ) as tp_table_23 (column_0, column_1, column_2) ON (tp_table_22.column_0 = tp_table_23.column_0)
)
UNION ALL
(
    SELECT
        tp_table_25.column_0,
        tp_table_28.column_1,
        tp_table_28.column_2
    FROM
        (
            SELECT
                tp_table_26.column_0,
                tp_table_27.column_1,
                tp_table_27.column_2
            FROM
                (
                    SELECT
                        top_level_tp_table_0.column_0::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
                ) as tp_table_26 (column_0)
                JOIN (
                    SELECT
                        combined_entry.column_0::bigint,
                        combined_entry.column_1::bigint,
                        combined_entry.column_2::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 5::bigint) AS combined_entry
                ) as tp_table_27 (column_0, column_1, column_2) ON (tp_table_26.column_0 = tp_table_27.column_0)
        ) as tp_table_25 (column_0, column_1, column_2)
        JOIN (
            SELECT
                base_join_tp_table_3.column_0::bigint,
                base_join_tp_table_3.column_1::bigint,
                base_join_tp_table_3.column_2::bigint
            FROM
                traceprov_read_worker_layer (3::bigint, 1::bigint) AS base_join_tp_table_3
        ) as tp_table_28 (column_0, column_1, column_2) ON (
            tp_table_25.column_2 = tp_table_28.column_0
            AND tp_table_25.column_1 = 3
        )
)
UNION ALL
(
    SELECT
        tp_table_30.column_0,
        tp_table_31.column_1,
        tp_table_31.column_2
    FROM
        (
            SELECT
                top_level_tp_table_0.column_0::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
        ) as tp_table_30 (column_0)
        JOIN (
            SELECT
                base_join_tp_table_3.column_0::bigint,
                base_join_tp_table_3.column_1::bigint,
                base_join_tp_table_3.column_2::bigint
            FROM
                traceprov_read_worker_layer (3::bigint, 1::bigint) AS base_join_tp_table_3
        ) as tp_table_31 (column_0, column_1, column_2) ON (tp_table_30.column_0 = tp_table_31.column_0)
)
UNION ALL
(
    SELECT
        tp_table_33.column_0,
        tp_table_36.column_1,
        tp_table_36.column_2
    FROM
        (
            SELECT
                tp_table_34.column_0,
                tp_table_35.column_1,
                tp_table_35.column_2
            FROM
                (
                    SELECT
                        top_level_tp_table_0.column_0::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
                ) as tp_table_34 (column_0)
                JOIN (
                    SELECT
                        combined_entry.column_0::bigint,
                        combined_entry.column_1::bigint,
                        combined_entry.column_2::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 5::bigint) AS combined_entry
                ) as tp_table_35 (column_0, column_1, column_2) ON (tp_table_34.column_0 = tp_table_35.column_0)
        ) as tp_table_33 (column_0, column_1, column_2)
        JOIN (
            SELECT
                base_join_tp_table_4.column_0::bigint,
                base_join_tp_table_4.column_1::bigint,
                base_join_tp_table_4.column_2::bigint
            FROM
                traceprov_read_worker_layer (4::bigint, 1::bigint) AS base_join_tp_table_4
        ) as tp_table_36 (column_0, column_1, column_2) ON (
            tp_table_33.column_2 = tp_table_36.column_0
            AND tp_table_33.column_1 = 4
        )
)
UNION ALL
(
    SELECT
        tp_table_38.column_0,
        tp_table_39.column_1,
        tp_table_39.column_2
    FROM
        (
            SELECT
                top_level_tp_table_0.column_0::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
        ) as tp_table_38 (column_0)
        JOIN (
            SELECT
                base_join_tp_table_4.column_0::bigint,
                base_join_tp_table_4.column_1::bigint,
                base_join_tp_table_4.column_2::bigint
            FROM
                traceprov_read_worker_layer (4::bigint, 1::bigint) AS base_join_tp_table_4
        ) as tp_table_39 (column_0, column_1, column_2) ON (tp_table_38.column_0 = tp_table_39.column_0)
)
UNION ALL
(
    SELECT
        tp_table_41.column_0,
        tp_table_44.column_1,
        tp_table_44.column_2
    FROM
        (
            SELECT
                tp_table_42.column_0,
                tp_table_43.column_1,
                tp_table_43.column_2
            FROM
                (
                    SELECT
                        top_level_tp_table_0.column_0::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
                ) as tp_table_42 (column_0)
                JOIN (
                    SELECT
                        combined_entry.column_0::bigint,
                        combined_entry.column_1::bigint,
                        combined_entry.column_2::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 5::bigint) AS combined_entry
                ) as tp_table_43 (column_0, column_1, column_2) ON (tp_table_42.column_0 = tp_table_43.column_0)
        ) as tp_table_41 (column_0, column_1, column_2)
        JOIN (
            SELECT
                base_join_tp_table_5.column_0::bigint,
                base_join_tp_table_5.column_1::bigint,
                base_join_tp_table_5.column_2::bigint
            FROM
                traceprov_read_worker_layer (5::bigint, 1::bigint) AS base_join_tp_table_5
        ) as tp_table_44 (column_0, column_1, column_2) ON (
            tp_table_41.column_2 = tp_table_44.column_0
            AND tp_table_41.column_1 = 5
        )
)
UNION ALL
(
    SELECT
        tp_table_46.column_0,
        tp_table_47.column_1,
        tp_table_47.column_2
    FROM
        (
            SELECT
                top_level_tp_table_0.column_0::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
        ) as tp_table_46 (column_0)
        JOIN (
            SELECT
                base_join_tp_table_5.column_0::bigint,
                base_join_tp_table_5.column_1::bigint,
                base_join_tp_table_5.column_2::bigint
            FROM
                traceprov_read_worker_layer (5::bigint, 1::bigint) AS base_join_tp_table_5
        ) as tp_table_47 (column_0, column_1, column_2) ON (tp_table_46.column_0 = tp_table_47.column_0)
)
UNION ALL
(
    SELECT
        tp_table_49.column_0,
        tp_table_52.column_1,
        tp_table_52.column_2
    FROM
        (
            SELECT
                tp_table_50.column_0,
                tp_table_51.column_1,
                tp_table_51.column_2
            FROM
                (
                    SELECT
                        top_level_tp_table_0.column_0::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
                ) as tp_table_50 (column_0)
                JOIN (
                    SELECT
                        combined_entry.column_0::bigint,
                        combined_entry.column_1::bigint,
                        combined_entry.column_2::bigint
                    FROM
                        traceprov_read_worker_layer (1::bigint, 5::bigint) AS combined_entry
                ) as tp_table_51 (column_0, column_1, column_2) ON (tp_table_50.column_0 = tp_table_51.column_0)
        ) as tp_table_49 (column_0, column_1, column_2)
        JOIN (
            SELECT
                base_join_tp_table_6.column_0::bigint,
                base_join_tp_table_6.column_1::bigint,
                base_join_tp_table_6.column_2::bigint
            FROM
                traceprov_read_worker_layer (6::bigint, 1::bigint) AS base_join_tp_table_6
        ) as tp_table_52 (column_0, column_1, column_2) ON (
            tp_table_49.column_2 = tp_table_52.column_0
            AND tp_table_49.column_1 = 6
        )
)
UNION ALL
(
    SELECT
        tp_table_54.column_0,
        tp_table_55.column_1,
        tp_table_55.column_2
    FROM
        (
            SELECT
                top_level_tp_table_0.column_0::bigint
            FROM
                traceprov_read_worker_layer (1::bigint, 2::bigint) AS top_level_tp_table_0
        ) as tp_table_54 (column_0)
        JOIN (
            SELECT
                base_join_tp_table_6.column_0::bigint,
                base_join_tp_table_6.column_1::bigint,
                base_join_tp_table_6.column_2::bigint
            FROM
                traceprov_read_worker_layer (6::bigint, 1::bigint) AS base_join_tp_table_6
        ) as tp_table_55 (column_0, column_1, column_2) ON (tp_table_54.column_0 = tp_table_55.column_0)
)) to '/home/realvinayak123/projects/traceprov/duckdb/playground/tpch/queries/root/1/infer_1_out.csv'