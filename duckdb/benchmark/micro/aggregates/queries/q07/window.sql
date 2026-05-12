WITH temp_view_2 AS (
    SELECT
        /*+ materialize */
        F0_0.ID AS ID,
        F0_0.Z AS Z,
        F0_0.VAL AS VAL,
        F0_0.ID AS "PROV_SKEW__1__0__NUM__1000000_ID",
        F0_0.ID AS _RESULT_TID,
        1 AS _SETPROV_DUP_COUNT
    FROM "SKEW_1_0_NUM_ROW_COUNT" F0_0
),
temp_view_1 AS (
    SELECT
        /*+ materialize */
        F0_0."AGGR_0" AS "AGGR_0",
        F0_0."AGG_GB_ARG1" AS "GROUP_0",
        F0_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID",
        DENSE_RANK() OVER (
            ORDER BY F0_0."AGG_GB_ARG1"
        ) AS _RESULT_TID,
        ROW_NUMBER() OVER (
            PARTITION BY F0_0."AGG_GB_ARG1"
            ORDER BY F0_0."AGG_GB_ARG1"
        ) AS _SETPROV_DUP_COUNT
    FROM (
            SELECT 1 AS "AGG_GB_ARG0",
                F0_0.Z AS "AGG_GB_ARG1",
                F0_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID",
                F0_0._RESULT_TID AS _RESULT_TID,
                F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT,
                COUNT(1) OVER (PARTITION BY F0_0.Z) AS "AGGR_0"
            FROM (
                    SELECT *
                    FROM temp_view_2
                ) F0_0
        ) F0_0
),
temp_view_0 AS (
    SELECT
        /*+ materialize */
        F0_0.C AS C,
        F0_0.Z AS Z,
        F0_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID"
    FROM (
            SELECT F0_0.C AS C,
                F0_0.Z AS Z,
                F0_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID",
                DENSE_RANK() OVER (
                    ORDER BY F0_0.C DESC NULLS LAST,
                        F0_0._RESULT_TID
                ) AS _RESULT_TID,
                F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
            FROM (
                    SELECT F0_0."AGGR_0" AS C,
                        F0_0."GROUP_0" AS Z,
                        F0_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID",
                        F0_0._RESULT_TID AS _RESULT_TID,
                        F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
                    FROM (
                            SELECT *
                            FROM temp_view_1
                        ) F0_0
                    ORDER BY C DESC NULLS LAST
                ) F0_0
        ) F0_0
    WHERE (F0_0._RESULT_TID <= 10)
)
SELECT F0_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID"
FROM (
        SELECT *
        FROM temp_view_0
    ) F0_0;