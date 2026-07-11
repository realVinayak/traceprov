SELECT
    F0_0."prov_data__table__1__000__000__random_id" AS "prov_data__table__1__000__000__random_id",
    F0_0.group_number AS group_number
FROM
    (
        SELECT
            F0_0.group_number AS group_number,
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
                    F0_0."GROUP_0" AS group_number,
                    F1_0."prov_data__table__1__000__000__random_id" AS "prov_data__table__1__000__000__random_id",
                    dense_rank() OVER (
                        ORDER BY
                            F0_0."GROUP_0"
                    ) AS _result_tid
                FROM
                    (
                        (
                            SELECT
                                min(F0_0.min_value) AS "AGGR_0",
                                F0_0.group_number AS "GROUP_0"
                            FROM
                                "data_table_ROW_COUNT_random" F0_0
                            GROUP BY
                                F0_0.group_number
                        ) F0_0
                        JOIN (
                            SELECT
                                F0_0.group_number AS "_P_SIDE_GROUP_0",
                                F0_0.id AS "prov_data__table__1__000__000__random_id"
                            FROM
                                "data_table_ROW_COUNT_random" F0_0
                        ) F1_0 ON (
                            (
                                F0_0."GROUP_0" IS NOT DISTINCT
                                FROM
                                    F1_0."_P_SIDE_GROUP_0"
                            )
                        )
                    )
                ORDER BY
                    min_over_group ASC NULLS LAST
            ) F0_0
    ) F0_0
WHERE
    (F0_0._result_tid <= :top_k_limit);