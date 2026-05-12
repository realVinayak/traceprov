SELECT F1_0."PROV_DATA__TABLE__1__000__000__RANDOM_ID" AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID"
FROM (
        (
            SELECT F0_0.GROUP_NUMBER AS "GROUP_0"
            FROM (
                    SELECT F0_0.NEGATIVE_GROUP_NUMBER AS NEGATIVE_GROUP_NUMBER,
                        F0_0.GROUP_NUMBER AS GROUP_NUMBER
                    FROM "DATA_TABLE_ROW_COUNT_RANDOM" F0_0
                ) F0_0
            WHERE (F0_0.NEGATIVE_GROUP_NUMBER >= :selectivity)
            GROUP BY F0_0.GROUP_NUMBER
        ) F0_0
        JOIN (
            SELECT F0_0.GROUP_NUMBER AS "_P_SIDE_GROUP_0",
                F0_0."PROV_DATA__TABLE__1__000__000__RANDOM_ID" AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID"
            FROM (
                    SELECT F0_0.NEGATIVE_GROUP_NUMBER AS NEGATIVE_GROUP_NUMBER,
                        F0_0.GROUP_NUMBER AS GROUP_NUMBER,
                        F0_0.ID AS "PROV_DATA__TABLE__1__000__000__RANDOM_ID"
                    FROM "DATA_TABLE_ROW_COUNT_RANDOM" F0_0
                ) F0_0
            WHERE (F0_0.NEGATIVE_GROUP_NUMBER >= :selectivity)
        ) F1_0 ON (
            (
                (F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")
                OR (
                    (F0_0."GROUP_0" IS NULL)
                    AND (F1_0."_P_SIDE_GROUP_0" IS NULL)
                )
            )
        )
    );