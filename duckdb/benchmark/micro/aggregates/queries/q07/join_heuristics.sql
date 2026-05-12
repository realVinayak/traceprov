SELECT F1_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID"
FROM (
        (
            SELECT COUNT(1) AS C,
                F0_0.Z AS Z
            FROM "SKEW_1_0_NUM_ROW_COUNT" F0_0
            GROUP BY F0_0.Z
            ORDER BY C DESC NULLS LAST
            LIMIT 10
        ) F0_0
        JOIN (
            SELECT F0_0."AGGR_0" AS C,
                F0_0."GROUP_0" AS Z,
                F1_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID"
            FROM (
                    (
                        SELECT COUNT(1) AS "AGGR_0",
                            F0_0.Z AS "GROUP_0"
                        FROM "SKEW_1_0_NUM_ROW_COUNT" F0_0
                        GROUP BY F0_0.Z
                    ) F0_0
                    JOIN (
                        SELECT F0_0.Z AS "_P_SIDE_GROUP_0",
                            F0_0.ID AS "PROV_SKEW__1__0__NUM__1000000_ID"
                        FROM "SKEW_1_0_NUM_ROW_COUNT" F0_0
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
            ORDER BY C DESC NULLS LAST
        ) F1_0 ON (
            (
                (F0_0.C = F1_0.C)
                AND (F0_0.Z = F1_0.Z)
            )
        )
    );