WITH temp_view_2 AS (
    SELECT
        /*+ materialize */
        F0_0.ID AS ID,
        F0_0.MIN_VALUE AS MIN_VALUE,
        F0_0.NEGATIVE_GROUP_NUMBER AS NEGATIVE_GROUP_NUMBER,
        F0_0.GROUP_NUMBER AS GROUP_NUMBER,
        F0_0.rowid AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID"
    FROM "DATA_TABLE_ROW_COUNT_RANDOM" F0_0
),
temp_view_1 AS (
    SELECT
        /*+ materialize */
        F0_0."AGGR_0" AS "AGGR_0",
        F0_0."GROUP_0" AS "GROUP_0",
        F1_0."PROV_DATA__TABLE__1__000__000__RANDOM_ID" AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID"
    FROM (
            (
                SELECT MIN(F0_0.MIN_VALUE) AS "AGGR_0",
                    F0_0.GROUP_NUMBER AS "GROUP_0"
                FROM "DATA_TABLE_ROW_COUNT_RANDOM" F0_0
                GROUP BY F0_0.GROUP_NUMBER
            ) F0_0
            JOIN (
                SELECT F0_0.GROUP_NUMBER AS "_P_SIDE_GROUP_0",
                    F0_0."PROV_DATA__TABLE__1__000__000__RANDOM_ID" AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID"
                FROM (
                        SELECT *
                        FROM temp_view_2
                    ) F0_0
            ) F1_0 ON (
                (
                    (F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")
                    OR (
                        (F0_0."GROUP_0" IS NULL)
                        AND (F1_0."_P_SIDE_GROUP_0" IS NULL)
                    )
                )
            )
        )
),
temp_view_0 AS (
    SELECT
        /*+ materialize */
        F0_0.MIN_OVER_GROUP AS MIN_OVER_GROUP,
        F0_0.GROUP_NUMBER AS GROUP_NUMBER,
        F1_0."PROV_DATA__TABLE__1__000__000__RANDOM_ID" AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID"
    FROM (
            (
                SELECT MIN(F0_0.MIN_VALUE) AS MIN_OVER_GROUP,
                    F0_0.GROUP_NUMBER AS GROUP_NUMBER
                FROM "DATA_TABLE_ROW_COUNT_RANDOM" F0_0
                GROUP BY F0_0.GROUP_NUMBER
                ORDER BY MIN_OVER_GROUP ASC NULLS LAST
                LIMIT :top_k_limit
            ) F0_0
            JOIN (
                SELECT F0_0."AGGR_0" AS MIN_OVER_GROUP,
                    F0_0."GROUP_0" AS GROUP_NUMBER,
                    F0_0."PROV_DATA__TABLE__1__000__000__RANDOM_ID" AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID"
                FROM (
                        SELECT *
                        FROM temp_view_1
                    ) F0_0
                ORDER BY MIN_OVER_GROUP ASC NULLS LAST
            ) F1_0 ON (
                (
                    (F0_0.MIN_OVER_GROUP = F1_0.MIN_OVER_GROUP)
                    AND (F0_0.GROUP_NUMBER = F1_0.GROUP_NUMBER)
                )
            )
        )
)
SELECT F0_0."PROV_DATA__TABLE__1__000__000__RANDOM_ID" AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID"
FROM (
        SELECT *
        FROM temp_view_0
    ) F0_0;