SELECT F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
    F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
    F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
FROM (
        SELECT F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
            F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
            DENSE_RANK() OVER (
                ORDER BY F0_0.REVENUE DESC NULLS LAST,
                    F0_0.O_ORDERDATE ASC NULLS LAST,
                    F0_0._RESULT_TID
            ) AS _RESULT_TID
        FROM (
                SELECT F0_0."AGGR_0" AS REVENUE,
                    F0_0."AGG_GB_ARG2" AS O_ORDERDATE,
                    F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                    F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                    F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                    DENSE_RANK() OVER (
                        ORDER BY F0_0."AGG_GB_ARG1",
                            F0_0."AGG_GB_ARG2",
                            F0_0."AGG_GB_ARG3"
                    ) AS _RESULT_TID
                FROM (
                        SELECT (F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT)) AS "AGG_GB_ARG0",
                            F0_0.L_ORDERKEY AS "AGG_GB_ARG1",
                            F0_0.O_ORDERDATE AS "AGG_GB_ARG2",
                            F0_0.O_SHIPPRIORITY AS "AGG_GB_ARG3",
                            F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                            F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                            SUM((F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT))) OVER (
                                PARTITION BY F0_0.L_ORDERKEY,
                                F0_0.O_ORDERDATE,
                                F0_0.O_SHIPPRIORITY
                            ) AS "AGGR_0"
                        FROM (
                                SELECT F0_0.C_CUSTKEY AS C_CUSTKEY,
                                    F0_0.C_MKTSEGMENT AS C_MKTSEGMENT,
                                    F0_0.O_ORDERKEY AS O_ORDERKEY,
                                    F0_0.O_CUSTKEY AS O_CUSTKEY,
                                    F0_0.O_ORDERDATE AS O_ORDERDATE,
                                    F0_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
                                    F1_0.L_ORDERKEY AS L_ORDERKEY,
                                    F1_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                                    F1_0.L_DISCOUNT AS L_DISCOUNT,
                                    F1_0.L_SHIPDATE AS L_SHIPDATE,
                                    F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                                    F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                                    F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
                                FROM (
                                        (
                                            SELECT F0_0.C_CUSTKEY AS C_CUSTKEY,
                                                F0_0.C_MKTSEGMENT AS C_MKTSEGMENT,
                                                F1_0.O_ORDERKEY AS O_ORDERKEY,
                                                F1_0.O_CUSTKEY AS O_CUSTKEY,
                                                F1_0.O_ORDERDATE AS O_ORDERDATE,
                                                F1_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
                                                F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                                                F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY
                                            FROM (
                                                    (
                                                        SELECT F0_0.C_CUSTKEY AS C_CUSTKEY,
                                                            F0_0.C_MKTSEGMENT AS C_MKTSEGMENT,
                                                            F0_0.rowid AS PROV_CUSTOMER_C__CUSTKEY
                                                        FROM CUSTOMER F0_0
                                                    ) F0_0
                                                    CROSS JOIN (
                                                        SELECT F0_0.O_ORDERKEY AS O_ORDERKEY,
                                                            F0_0.O_CUSTKEY AS O_CUSTKEY,
                                                            F0_0.O_ORDERDATE AS O_ORDERDATE,
                                                            F0_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
                                                            F0_0.rowid AS PROV_ORDERS_O__ORDERKEY
                                                        FROM ORDERS F0_0
                                                    ) F1_0
                                                )
                                        ) F0_0
                                        CROSS JOIN (
                                            SELECT F0_0.L_ORDERKEY AS L_ORDERKEY,
                                                F0_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                                                F0_0.L_DISCOUNT AS L_DISCOUNT,
                                                F0_0.L_SHIPDATE AS L_SHIPDATE,
                                                F0_0.rowid AS PROV_LINEITEM_L__ORDERKEY
                                            FROM LINEITEM F0_0
                                        ) F1_0
                                    )
                            ) F0_0
                        WHERE (
                                (
                                    (
                                        (
                                            (F0_0.C_MKTSEGMENT = 'BUILDING')
                                            AND (F0_0.C_CUSTKEY = F0_0.O_CUSTKEY)
                                        )
                                        AND (F0_0.L_ORDERKEY = F0_0.O_ORDERKEY)
                                    )
                                    AND (F0_0.O_ORDERDATE < '1995-03-15')
                                )
                                AND (F0_0.L_SHIPDATE > '1995-03-15')
                            )
                    ) F0_0
                ORDER BY REVENUE DESC NULLS LAST,
                    O_ORDERDATE ASC NULLS LAST
            ) F0_0
    ) F0_0
WHERE (F0_0._RESULT_TID <= 10);