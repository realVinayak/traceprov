WITH temp_view_1 AS (
    SELECT
        /*+ materialize */
        F0_0."AGGR_0" AS "AGGR_0",
        F0_0.GROUP_NUMBER AS "GROUP_0",
        F0_0."PROV_DATA__TABLE__1__000__000__RANDOM_ID" AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID",
        DENSE_RANK() OVER (
            ORDER BY F0_0.GROUP_NUMBER
        ) AS _RESULT_TID,
        ROW_NUMBER() OVER (
            PARTITION BY F0_0.GROUP_NUMBER
            ORDER BY F0_0.GROUP_NUMBER
        ) AS _SETPROV_DUP_COUNT
    FROM (
            SELECT F0_0.ID AS ID,
                F0_0.MIN_VALUE AS MIN_VALUE,
                F0_0.NEGATIVE_GROUP_NUMBER AS NEGATIVE_GROUP_NUMBER,
                F0_0.GROUP_NUMBER AS GROUP_NUMBER,
                F0_0.rowid AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID",
                F0_0.ID AS _RESULT_TID,
                1 AS _SETPROV_DUP_COUNT,
                MIN(F0_0.MIN_VALUE) OVER (PARTITION BY F0_0.GROUP_NUMBER) AS "AGGR_0"
            FROM "DATA_TABLE_ROW_COUNT_RANDOM" F0_0
        ) F0_0
),
temp_view_0 AS (
    SELECT
        /*+ materialize */
        F0_0.MIN_OVER_GROUP AS MIN_OVER_GROUP,
        F0_0.GROUP_NUMBER AS GROUP_NUMBER,
        F0_0."PROV_DATA__TABLE__1__000__000__RANDOM_ID" AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID"
    FROM (
            SELECT F0_0.MIN_OVER_GROUP AS MIN_OVER_GROUP,
                F0_0.GROUP_NUMBER AS GROUP_NUMBER,
                F0_0."PROV_DATA__TABLE__1__000__000__RANDOM_ID" AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID",
                DENSE_RANK() OVER (
                    ORDER BY F0_0.MIN_OVER_GROUP ASC NULLS LAST,
                        F0_0._RESULT_TID
                ) AS _RESULT_TID,
                F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
            FROM (
                    SELECT F0_0."AGGR_0" AS MIN_OVER_GROUP,
                        F0_0."GROUP_0" AS GROUP_NUMBER,
                        F0_0."PROV_DATA__TABLE__1__000__000__RANDOM_ID" AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID",
                        F0_0._RESULT_TID AS _RESULT_TID,
                        F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
                    FROM (
                            SELECT *
                            FROM temp_view_1
                        ) F0_0
                    ORDER BY MIN_OVER_GROUP ASC NULLS LAST
                ) F0_0
        ) F0_0
    WHERE (F0_0._RESULT_TID <= :top_k_limit)
)
SELECT F0_0."PROV_DATA__TABLE__1__000__000__RANDOM_ID" AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID"
FROM (
        SELECT *
        FROM temp_view_0
    ) F0_0;