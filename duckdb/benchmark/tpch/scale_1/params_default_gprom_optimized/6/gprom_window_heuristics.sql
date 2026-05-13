SELECT F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
FROM (
        SELECT F0_0."AGG_GB_ARG0" AS "AGG_GB_ARG0",
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
            F0_0._RESULT_TID AS _RESULT_TID,
            COUNT(1) OVER () AS __DUMMY_CNT
        FROM (
                (
                    SELECT (F0_0.L_EXTENDEDPRICE * F0_0.L_DISCOUNT) AS "AGG_GB_ARG0",
                        F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                        F0_0._RESULT_TID AS _RESULT_TID
                    FROM (
                            SELECT F0_0.L_QUANTITY AS L_QUANTITY,
                                F0_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                                F0_0.L_DISCOUNT AS L_DISCOUNT,
                                F0_0.L_SHIPDATE AS L_SHIPDATE,
                                F0_0.rowid AS PROV_LINEITEM_L__ORDERKEY,
                                F0_0.rowid AS _RESULT_TID
                            FROM LINEITEM F0_0
                        ) F0_0
                    WHERE (
                            (
                                (
                                    (
                                        (F0_0.L_SHIPDATE >= '1994-01-01')
                                        AND (F0_0.L_SHIPDATE < '1995-01-01')
                                    )
                                    AND (F0_0.L_DISCOUNT >= 0.050000)
                                )
                                AND (F0_0.L_DISCOUNT <= 0.070000)
                            )
                            AND (F0_0.L_QUANTITY < 24)
                        )
                    UNION ALL
                    (
                        SELECT NULL AS "AGG_GB_ARG0",
                            NULL AS PROV_LINEITEM_L__ORDERKEY,
                            -1 AS _RESULT_TID
                    )
                )
            ) F0_0
    ) F0_0
WHERE (
        (F0_0.__DUMMY_CNT = 1)
        OR (F0_0._RESULT_TID <> -1)
    );