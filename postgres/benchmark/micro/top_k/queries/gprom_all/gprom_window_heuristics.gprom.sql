SELECT
    F0_0."prov_data__table__1__000__000__random_id" AS "prov_data__table__1__000__000__random_id"
FROM
    (
        SELECT
            F0_0."prov_data__table__1__000__000__random_id" AS "prov_data__table__1__000__000__random_id",
            dense_rank() OVER (
                ORDER BY
                    F0_0.min_over_group ASC NULLS LAST,
                    F0_0._result_tid
            ) AS _result_tid
        FROM
            (
                SELECT
                    F0_0."AGGR_0" AS min_over_group,
                    F0_0."prov_data__table__1__000__000__random_id" AS "prov_data__table__1__000__000__random_id",
                    dense_rank() OVER (
                        ORDER BY
                            F0_0.group_number
                    ) AS _result_tid
                FROM
                    (
                        SELECT
                            F0_0.min_value AS min_value,
                            F0_0.group_number AS group_number,
                            F0_0.id AS "prov_data__table__1__000__000__random_id",
                            min(F0_0.min_value) OVER (PARTITION BY F0_0.group_number) AS "AGGR_0"
                        FROM
                            "data_table_ROW_COUNT_random" F0_0
                    ) F0_0
                ORDER BY
                    min_over_group ASC NULLS LAST
            ) F0_0
    ) F0_0
WHERE
    (F0_0._result_tid <= :top_k_limit);