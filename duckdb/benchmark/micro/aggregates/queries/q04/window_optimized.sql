WITH temp_view_1 AS (
    SELECT
        /*+ materialize */
        F0_0."AGGR_0" AS "AVG(VAL)",
        F0_0."GROUP_0" AS Z,
        F0_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID",
        F0_0._RESULT_TID AS _RESULT_TID,
        F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM (
            SELECT F0_0."AGGR_0" AS "AGGR_0",
                F0_0."AGGR_1" AS "AGGR_1",
                F0_0.Z AS "GROUP_0",
                F0_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID",
                DENSE_RANK() OVER (
                    ORDER BY F0_0.Z
                ) AS _RESULT_TID,
                ROW_NUMBER() OVER (
                    PARTITION BY F0_0.Z
                    ORDER BY F0_0.Z
                ) AS _SETPROV_DUP_COUNT
            FROM (
                    SELECT F0_0.ID AS ID,
                        F0_0.Z AS Z,
                        F0_0.VAL AS VAL,
                        F0_0.rowid AS "PROV_SKEW__1__0__NUM__1000000_ID",
                        F0_0.ID AS _RESULT_TID,
                        1 AS _SETPROV_DUP_COUNT,
                        AVG(F0_0.VAL) OVER (PARTITION BY F0_0.Z) AS "AGGR_0",
                        AVG(F0_0.VAL) OVER (PARTITION BY F0_0.Z) AS "AGGR_1"
                    FROM "SKEW_1_0_NUM_ROW_COUNT" F0_0
                ) F0_0
        ) F0_0
    WHERE (F0_0."AGGR_0" > 50)
),
temp_view_0 AS (
    SELECT
        /*+ materialize */
        F0_0."AVG(VAL)" AS "AVG(VAL)",
        F0_0.Z AS Z,
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