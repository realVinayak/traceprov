with layer_1 as (
    (
        SELECT
            tp_table_5.column_0,
            tp_table_6.column_1 as terminal_column_1,
            tp_table_6.column_2 as terminal_column_2,
            tp_table_6.column_3 as terminal_column_3
        FROM
            (
                SELECT
                    intermediate_join_tp_table_1.column_0,
                    intermediate_join_tp_table_1.column_1,
                    intermediate_join_tp_table_1.column_2,
                    intermediate_join_tp_table_1.column_3
                FROM
                    traceprov_read_worker_layer(0 :: bigint, 0 :: int, 1 :: int) AS intermediate_join_tp_table_1
            ) as tp_table_6(column_0, column_1, column_2, column_3)
            JOIN (
                SELECT
                    top_level_tp_table_0.column_0
                FROM
                    traceprov_read_worker_layer(0 :: bigint, 0 :: int, 2 :: int) AS top_level_tp_table_0
            ) as tp_table_5(column_0) ON (tp_table_5.column_0 = tp_table_6.column_0)
    )
    UNION
    ALL (
        SELECT
            tp_table_8.column_0,
            tp_table_10.column_1 as terminal_column_1,
            tp_table_10.column_2 as terminal_column_2,
            tp_table_10.column_3 as terminal_column_3
        FROM
            (
                SELECT
                    base_join_tp_table_2.column_0,
                    base_join_tp_table_2.column_1,
                    base_join_tp_table_2.column_2,
                    base_join_tp_table_2.column_3
                FROM
                    traceprov_read_worker_layer(0 :: bigint, 0 :: int, 1 :: int) AS base_join_tp_table_2
            ) as tp_table_10(column_0, column_1, column_2, column_3)
            JOIN (
                SELECT
                    tp_table_5.column_0,
                    tp_table_9.column_1
                FROM
                    (
                        SELECT
                            combined_entry.column_0,
                            combined_entry.column_1
                        FROM
                            traceprov_read_worker_layer(1 :: bigint, 0 :: int, 1 :: int) AS combined_entry
                    ) as tp_table_9(column_0, column_1)
                    JOIN (
                        SELECT
                            top_level_tp_table_0.column_0
                        FROM
                            traceprov_read_worker_layer(0 :: bigint, 0 :: int, 2 :: int) AS top_level_tp_table_0
                    ) as tp_table_5(column_0) ON (tp_table_5.column_0 = tp_table_9.column_0)
            ) as tp_table_8(column_0, column_1) ON (tp_table_8.column_1 = tp_table_10.column_0)
    )
)
select
    string_agg(
        '(' || terminal_column_1 :: text || ' ⊗ ' || terminal_column_2 :: text || ' ⊗ ' || terminal_column_3 :: text || ')',
        ' ⊕ '
    ) as provsql
from
    layer_1