SELECT F0_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID"
FROM (
        SELECT F0_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID",
            DENSE_RANK() OVER (
                ORDER BY F0_0.C DESC NULLS LAST,
                    F0_0._RESULT_TID
            ) AS _RESULT_TID
        FROM (
                SELECT F0_0."AGGR_0" AS C,
                    F0_0."PROV_SKEW__1__0__NUM__1000000_ID" AS "PROV_SKEW__1__0__NUM__1000000_ID",
                    DENSE_RANK() OVER (
                        ORDER BY F0_0."AGG_GB_ARG1"
                    ) AS _RESULT_TID
                FROM (
                        SELECT 1 AS "AGG_GB_ARG0",
                            F0_0.Z AS "AGG_GB_ARG1",
                            F0_0.ID AS "PROV_SKEW__1__0__NUM__1000000_ID",
                            COUNT(1) OVER (PARTITION BY F0_0.Z) AS "AGGR_0"
                        FROM "SKEW_1_0_NUM_ROW_COUNT" F0_0
                    ) F0_0
                ORDER BY C DESC NULLS LAST
            ) F0_0
    ) F0_0
WHERE (F0_0._RESULT_TID <= 10);