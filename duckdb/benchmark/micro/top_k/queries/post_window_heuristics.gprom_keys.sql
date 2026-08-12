SELECT
    F0_0.GROUP_NUMBER AS GROUP_NUMBER,
    F0_0."PROV_DATA__TABLE__1__000__000__RANDOM_ID" AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID"
FROM
    (
        SELECT
            F0_0.GROUP_NUMBER AS GROUP_NUMBER,
            F0_0."PROV_DATA__TABLE__1__000__000__RANDOM_ID" AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID",
            DENSE_RANK() OVER (
                ORDER BY
                    F0_0.MIN_OVER_GROUP ASC NULLS LAST,
                    F0_0._RESULT_TID
            ) AS _RESULT_TID
        FROM
            (
                SELECT
                    F0_0."AGGR_0" AS MIN_OVER_GROUP,
                    F0_0.GROUP_NUMBER AS GROUP_NUMBER,
                    F0_0."PROV_DATA__TABLE__1__000__000__RANDOM_ID" AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID",
                    DENSE_RANK() OVER (
                        ORDER BY
                            F0_0.GROUP_NUMBER
                    ) AS _RESULT_TID
                FROM
                    (
                        SELECT
                            F0_0.MIN_VALUE AS MIN_VALUE,
                            F0_0.GROUP_NUMBER AS GROUP_NUMBER,
                            F0_0.rowid AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID",
                            MIN(F0_0.MIN_VALUE) OVER (PARTITION BY F0_0.GROUP_NUMBER) AS "AGGR_0"
                        FROM
                            "DATA_TABLE_ROW_COUNT_RANDOM" F0_0
                    ) F0_0
                ORDER BY
                    MIN_OVER_GROUP ASC NULLS LAST
            ) F0_0
    ) F0_0
WHERE
    (F0_0._RESULT_TID <= :top_k_limit);