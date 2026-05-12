WITH temp_view_2 AS (
    SELECT
        /*+ materialize */
        F0_0.ID AS ID,
        F0_0.Z AS Z,
        F0_0.VAL AS VAL,
        F0_0.rowid AS "PROV_SKEW__1__0__NUM__1000000_ID"
    FROM "SKEW_1_0_NUM_ROW_COUNT" F0_0
),
temp_view_1 AS (
    SELECT
        /*+ materialize */
        F0_0."AGGR_0" AS "AGGR_0",
        F0_0."GROUP_0" AS "GROUP_0",
        F1_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID"
    FROM (
            (
                SELECT COUNT(F0_0.VAL) AS "AGGR_0",
                    F0_0.Z AS "GROUP_0"
                FROM "SKEW_1_0_NUM_ROW_COUNT" F0_0
                GROUP BY F0_0.Z
            ) F0_0
            JOIN (
                SELECT F0_0.Z AS "_P_SIDE_GROUP_0",
                    F0_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID"
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
        F0_0."AGGR_0" AS "COUNT(VAL)",
        F0_0."GROUP_0" AS Z,
        F0_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID"
    FROM (
            SELECT *
            FROM temp_view_1
        ) F0_0
)
SELECT F0_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID"
FROM (
        SELECT *
        FROM temp_view_0
    ) F0_0;