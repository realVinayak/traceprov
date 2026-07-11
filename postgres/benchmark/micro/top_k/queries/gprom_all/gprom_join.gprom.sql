WITH temp_view_2 AS (
    SELECT
        /*+ materialize */
        F0_0.id AS id,
        F0_0.min_value AS min_value,
        F0_0.negative_group_number AS negative_group_number,
        F0_0.group_number AS group_number,
        F0_0.id AS "prov_data__table__1__000__000__random_id"
    FROM
        "data_table_ROW_COUNT_random" F0_0
),
temp_view_1 AS (
    SELECT
        /*+ materialize */
        F0_0."AGGR_0" AS "AGGR_0",
        F0_0."GROUP_0" AS "GROUP_0",
        F1_0."prov_data__table__1__000__000__random_id" AS "prov_data__table__1__000__000__random_id"
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
                    F0_0."prov_data__table__1__000__000__random_id" AS "prov_data__table__1__000__000__random_id"
                FROM
                    (
                        SELECT
                            *
                        FROM
                            temp_view_2
                    ) F0_0
            ) F1_0 ON (
                (
                    F0_0."GROUP_0" IS NOT DISTINCT
                    FROM
                        F1_0."_P_SIDE_GROUP_0"
                )
            )
        )
),
temp_view_0 AS (
    SELECT
        /*+ materialize */
        F0_0.min_over_group AS min_over_group,
        F0_0.group_number AS group_number,
        F1_0."prov_data__table__1__000__000__random_id" AS "prov_data__table__1__000__000__random_id"
    FROM
        (
            (
                SELECT
                    min(F0_0.min_value) AS min_over_group,
                    F0_0.group_number AS group_number
                FROM
                    "data_table_ROW_COUNT_random" F0_0
                GROUP BY
                    F0_0.group_number
                ORDER BY
                    min_over_group ASC NULLS LAST
                LIMIT
                    :top_k_limit
            ) F0_0
            JOIN (
                SELECT
                    F0_0."AGGR_0" AS min_over_group,
                    F0_0."GROUP_0" AS group_number,
                    F0_0."prov_data__table__1__000__000__random_id" AS "prov_data__table__1__000__000__random_id"
                FROM
                    (
                        SELECT
                            *
                        FROM
                            temp_view_1
                    ) F0_0
                ORDER BY
                    min_over_group ASC NULLS LAST
            ) F1_0 ON (
                (
                    (F0_0.min_over_group = F1_0.min_over_group)
                    AND (F0_0.group_number = F1_0.group_number)
                )
            )
        )
)
SELECT
    F0_0."prov_data__table__1__000__000__random_id" AS "prov_data__table__1__000__000__random_id"
FROM
    (
        SELECT
            *
        FROM
            temp_view_0
    ) F0_0;