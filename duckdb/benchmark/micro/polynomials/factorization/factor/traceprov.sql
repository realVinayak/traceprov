with layer_1 as (
    (
        SELECT
            tp_table_9.column_0,
            tp_table_9.column_1,
            tp_table_9.column_2,
            tp_table_10.column_1 as terminal_column_1
        FROM
            (
                SELECT
                    intermediate_join_tp_table_1.column_0,
                    intermediate_join_tp_table_1.column_1
                FROM
                    traceprov_read_worker_layer(0 :: bigint, 0 :: int, 1 :: int) AS intermediate_join_tp_table_1
            ) as tp_table_10(column_0, column_1)
            JOIN (
                SELECT
                    top_level_tp_table_0.column_0,
                    top_level_tp_table_0.column_1,
                    top_level_tp_table_0.column_2
                FROM
                    traceprov_read_worker_layer(0 :: bigint, 0 :: int, 4 :: int) AS top_level_tp_table_0
            ) as tp_table_9(column_0, column_1, column_2) ON (tp_table_9.column_0 = tp_table_10.column_0)
    )
    UNION
    ALL (
        SELECT
            tp_table_12.column_0,
            tp_table_12.column_1,
            tp_table_12.column_2,
            tp_table_14.column_1 as terminal_column_1
        FROM
            (
                SELECT
                    base_join_tp_table_2.column_0,
                    base_join_tp_table_2.column_1
                FROM
                    traceprov_read_worker_layer(0 :: bigint, 0 :: int, 1 :: int) AS base_join_tp_table_2
            ) as tp_table_14(column_0, column_1)
            JOIN (
                SELECT
                    tp_table_9.column_0,
                    tp_table_9.column_1,
                    tp_table_9.column_2,
                    tp_table_13.column_1
                FROM
                    (
                        SELECT
                            combined_entry.column_0,
                            combined_entry.column_1
                        FROM
                            traceprov_read_worker_layer(1 :: bigint, 0 :: int, 1 :: int) AS combined_entry
                    ) as tp_table_13(column_0, column_1)
                    JOIN (
                        SELECT
                            top_level_tp_table_0.column_0,
                            top_level_tp_table_0.column_1,
                            top_level_tp_table_0.column_2
                        FROM
                            traceprov_read_worker_layer(0 :: bigint, 0 :: int, 4 :: int) AS top_level_tp_table_0
                    ) as tp_table_9(column_0, column_1, column_2) ON (tp_table_9.column_0 = tp_table_13.column_0)
            ) as tp_table_12(column_0, column_1, column_2, column_3) ON (tp_table_12.column_3 = tp_table_14.column_0)
    )
),
layer_2 as (
    (
        SELECT
            tp_table_9.column_0,
            tp_table_9.column_1,
            tp_table_9.column_2,
            tp_table_10.column_1 as terminal_column_1
        FROM
            (
                SELECT
                    intermediate_join_tp_table_1.column_0,
                    intermediate_join_tp_table_1.column_1
                FROM
                    traceprov_read_worker_layer(0 :: bigint, 0 :: int, 2 :: int) AS intermediate_join_tp_table_1
            ) as tp_table_10(column_0, column_1)
            JOIN (
                SELECT
                    top_level_tp_table_0.column_0,
                    top_level_tp_table_0.column_1,
                    top_level_tp_table_0.column_2
                FROM
                    traceprov_read_worker_layer(0 :: bigint, 0 :: int, 4 :: int) AS top_level_tp_table_0
            ) as tp_table_9(column_0, column_1, column_2) ON (tp_table_9.column_1 = tp_table_10.column_0)
    )
    UNION
    ALL (
        SELECT
            tp_table_12.column_0,
            tp_table_12.column_1,
            tp_table_12.column_2,
            tp_table_14.column_1 as terminal_column_1
        FROM
            (
                SELECT
                    base_join_tp_table_2.column_0,
                    base_join_tp_table_2.column_1
                FROM
                    traceprov_read_worker_layer(0 :: bigint, 0 :: int, 2 :: int) AS base_join_tp_table_2
            ) as tp_table_14(column_0, column_1)
            JOIN (
                SELECT
                    tp_table_9.column_0,
                    tp_table_9.column_1,
                    tp_table_9.column_2,
                    tp_table_13.column_1
                FROM
                    (
                        SELECT
                            combined_entry.column_0,
                            combined_entry.column_1
                        FROM
                            traceprov_read_worker_layer(1 :: bigint, 0 :: int, 2 :: int) AS combined_entry
                    ) as tp_table_13(column_0, column_1)
                    JOIN (
                        SELECT
                            top_level_tp_table_0.column_0,
                            top_level_tp_table_0.column_1,
                            top_level_tp_table_0.column_2
                        FROM
                            traceprov_read_worker_layer(0 :: bigint, 0 :: int, 4 :: int) AS top_level_tp_table_0
                    ) as tp_table_9(column_0, column_1, column_2) ON (tp_table_9.column_1 = tp_table_13.column_0)
            ) as tp_table_12(column_0, column_1, column_2, column_3) ON (tp_table_12.column_3 = tp_table_14.column_0)
    )
),
layer_3 as (
    (
        SELECT
            tp_table_9.column_0,
            tp_table_9.column_1,
            tp_table_9.column_2,
            tp_table_10.column_1 as terminal_column_1
        FROM
            (
                SELECT
                    intermediate_join_tp_table_1.column_0,
                    intermediate_join_tp_table_1.column_1
                FROM
                    traceprov_read_worker_layer(0 :: bigint, 0 :: int, 3 :: int) AS intermediate_join_tp_table_1
            ) as tp_table_10(column_0, column_1)
            JOIN (
                SELECT
                    top_level_tp_table_0.column_0,
                    top_level_tp_table_0.column_1,
                    top_level_tp_table_0.column_2
                FROM
                    traceprov_read_worker_layer(0 :: bigint, 0 :: int, 4 :: int) AS top_level_tp_table_0
            ) as tp_table_9(column_0, column_1, column_2) ON (tp_table_9.column_2 = tp_table_10.column_0)
    )
    UNION
    ALL (
        SELECT
            tp_table_12.column_0,
            tp_table_12.column_1,
            tp_table_12.column_2,
            tp_table_14.column_1 as terminal_column_1
        FROM
            (
                SELECT
                    base_join_tp_table_2.column_0,
                    base_join_tp_table_2.column_1
                FROM
                    traceprov_read_worker_layer(0 :: bigint, 0 :: int, 3 :: int) AS base_join_tp_table_2
            ) as tp_table_14(column_0, column_1)
            JOIN (
                SELECT
                    tp_table_9.column_0,
                    tp_table_9.column_1,
                    tp_table_9.column_2,
                    tp_table_13.column_1
                FROM
                    (
                        SELECT
                            combined_entry.column_0,
                            combined_entry.column_1
                        FROM
                            traceprov_read_worker_layer(1 :: bigint, 0 :: int, 3 :: int) AS combined_entry
                    ) as tp_table_13(column_0, column_1)
                    JOIN (
                        SELECT
                            top_level_tp_table_0.column_0,
                            top_level_tp_table_0.column_1,
                            top_level_tp_table_0.column_2
                        FROM
                            traceprov_read_worker_layer(0 :: bigint, 0 :: int, 4 :: int) AS top_level_tp_table_0
                    ) as tp_table_9(column_0, column_1, column_2) ON (tp_table_9.column_2 = tp_table_13.column_0)
            ) as tp_table_12(column_0, column_1, column_2, column_3) ON (tp_table_12.column_3 = tp_table_14.column_0)
    )
)
select
    '(' || concat_ws(
        ' ⊗ ',
        (
            select
                '(' || string_agg(terminal_column_1 :: text, ' ⊕ ') || ')'
            from
                layer_1
        ),
        (
            select
                '(' || string_agg(terminal_column_1 :: text, ' ⊕ ') || ')'
            from
                layer_2
        ),
        (
            select
                '(' || string_agg(terminal_column_1 :: text, ' ⊕ ') || ')'
            from
                layer_3
        )
    ) || ')' as prov