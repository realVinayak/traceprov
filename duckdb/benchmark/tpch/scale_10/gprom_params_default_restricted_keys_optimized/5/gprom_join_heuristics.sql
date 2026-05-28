SELECT
  F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
  F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
  F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
  F0_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY,
  F0_0.N_NAME AS N_NAME
FROM
  (
    SELECT
      F0_0."GROUP_0" AS N_NAME,
      F0_0."AGGR_0" AS REVENUE,
      F1_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F1_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
      F1_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY
    FROM
      (
        (
          SELECT
            SUM((F2_0.L_EXTENDEDPRICE * (1 - F2_0.L_DISCOUNT))) AS "AGGR_0",
            F4_0.N_NAME AS "GROUP_0"
          FROM
            (
              (
                (
                  (
                    (
                      (
                        SELECT
                          F0_0.C_CUSTKEY AS C_CUSTKEY,
                          F0_0.C_NATIONKEY AS C_NATIONKEY
                        FROM
                          CUSTOMER AS F0_0
                      ) AS F0_0
                      CROSS JOIN (
                        SELECT
                          F0_0.O_ORDERKEY AS O_ORDERKEY,
                          F0_0.O_CUSTKEY AS O_CUSTKEY,
                          F0_0.O_ORDERDATE AS O_ORDERDATE
                        FROM
                          ORDERS AS F0_0
                      ) AS F1_0
                    )
                    CROSS JOIN (
                      SELECT
                        F0_0.L_ORDERKEY AS L_ORDERKEY,
                        F0_0.L_SUPPKEY AS L_SUPPKEY,
                        F0_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                        F0_0.L_DISCOUNT AS L_DISCOUNT
                      FROM
                        LINEITEM AS F0_0
                    ) AS F2_0
                  )
                  CROSS JOIN (
                    SELECT
                      F0_0.S_SUPPKEY AS S_SUPPKEY,
                      F0_0.S_NATIONKEY AS S_NATIONKEY
                    FROM
                      SUPPLIER AS F0_0
                  ) AS F3_0
                )
                CROSS JOIN (
                  SELECT
                    F0_0.N_NATIONKEY AS N_NATIONKEY,
                    F0_0.N_NAME AS N_NAME,
                    F0_0.N_REGIONKEY AS N_REGIONKEY
                  FROM
                    NATION AS F0_0
                ) AS F4_0
              )
              CROSS JOIN (
                SELECT
                  F0_0.R_REGIONKEY AS R_REGIONKEY,
                  F0_0.R_NAME AS R_NAME
                FROM
                  REGION AS F0_0
              ) AS F5_0
            )
          WHERE
            (
              (
                (
                  (
                    (
                      (
                        (
                          (
                            (F0_0.C_CUSTKEY = F1_0.O_CUSTKEY)
                            AND (F2_0.L_ORDERKEY = F1_0.O_ORDERKEY)
                          )
                          AND (F2_0.L_SUPPKEY = F3_0.S_SUPPKEY)
                        )
                        AND (F0_0.C_NATIONKEY = F3_0.S_NATIONKEY)
                      )
                      AND (F3_0.S_NATIONKEY = F4_0.N_NATIONKEY)
                    )
                    AND (F4_0.N_REGIONKEY = F5_0.R_REGIONKEY)
                  )
                  AND (F5_0.R_NAME = 'ASIA')
                )
                AND (F1_0.O_ORDERDATE >= '1994-01-01')
              )
              AND (F1_0.O_ORDERDATE < '1995-01-01')
            )
          GROUP BY
            F4_0.N_NAME
        ) AS F0_0
        JOIN (
          SELECT
            F0_0.N_NAME AS "_P_SIDE_GROUP_0",
            F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
            F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
            F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
            F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
            F0_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY
          FROM
            (
              SELECT
                F0_0.C_CUSTKEY AS C_CUSTKEY,
                F0_0.C_NATIONKEY AS C_NATIONKEY,
                F0_0.O_ORDERKEY AS O_ORDERKEY,
                F0_0.O_CUSTKEY AS O_CUSTKEY,
                F0_0.O_ORDERDATE AS O_ORDERDATE,
                F0_0.L_ORDERKEY AS L_ORDERKEY,
                F0_0.L_SUPPKEY AS L_SUPPKEY,
                F0_0.S_SUPPKEY AS S_SUPPKEY,
                F0_0.S_NATIONKEY AS S_NATIONKEY,
                F0_0.N_NATIONKEY AS N_NATIONKEY,
                F0_0.N_NAME AS N_NAME,
                F0_0.N_REGIONKEY AS N_REGIONKEY,
                F1_0.R_REGIONKEY AS R_REGIONKEY,
                F1_0.R_NAME AS R_NAME,
                F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
                F1_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY
              FROM
                (
                  (
                    SELECT
                      F0_0.C_CUSTKEY AS C_CUSTKEY,
                      F0_0.C_NATIONKEY AS C_NATIONKEY,
                      F0_0.O_ORDERKEY AS O_ORDERKEY,
                      F0_0.O_CUSTKEY AS O_CUSTKEY,
                      F0_0.O_ORDERDATE AS O_ORDERDATE,
                      F0_0.L_ORDERKEY AS L_ORDERKEY,
                      F0_0.L_SUPPKEY AS L_SUPPKEY,
                      F0_0.S_SUPPKEY AS S_SUPPKEY,
                      F0_0.S_NATIONKEY AS S_NATIONKEY,
                      F1_0.N_NATIONKEY AS N_NATIONKEY,
                      F1_0.N_NAME AS N_NAME,
                      F1_0.N_REGIONKEY AS N_REGIONKEY,
                      F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                      F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY
                    FROM
                      (
                        (
                          SELECT
                            F0_0.C_CUSTKEY AS C_CUSTKEY,
                            F0_0.C_NATIONKEY AS C_NATIONKEY,
                            F0_0.O_ORDERKEY AS O_ORDERKEY,
                            F0_0.O_CUSTKEY AS O_CUSTKEY,
                            F0_0.O_ORDERDATE AS O_ORDERDATE,
                            F0_0.L_ORDERKEY AS L_ORDERKEY,
                            F0_0.L_SUPPKEY AS L_SUPPKEY,
                            F1_0.S_SUPPKEY AS S_SUPPKEY,
                            F1_0.S_NATIONKEY AS S_NATIONKEY,
                            F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                            F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                            F1_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY
                          FROM
                            (
                              (
                                SELECT
                                  F0_0.C_CUSTKEY AS C_CUSTKEY,
                                  F0_0.C_NATIONKEY AS C_NATIONKEY,
                                  F0_0.O_ORDERKEY AS O_ORDERKEY,
                                  F0_0.O_CUSTKEY AS O_CUSTKEY,
                                  F0_0.O_ORDERDATE AS O_ORDERDATE,
                                  F1_0.L_ORDERKEY AS L_ORDERKEY,
                                  F1_0.L_SUPPKEY AS L_SUPPKEY,
                                  F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                                  F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                                  F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
                                FROM
                                  (
                                    (
                                      SELECT
                                        F0_0.C_CUSTKEY AS C_CUSTKEY,
                                        F0_0.C_NATIONKEY AS C_NATIONKEY,
                                        F1_0.O_ORDERKEY AS O_ORDERKEY,
                                        F1_0.O_CUSTKEY AS O_CUSTKEY,
                                        F1_0.O_ORDERDATE AS O_ORDERDATE,
                                        F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                                        F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY
                                      FROM
                                        (
                                          (
                                            SELECT
                                              F0_0.C_CUSTKEY AS C_CUSTKEY,
                                              F0_0.C_NATIONKEY AS C_NATIONKEY,
                                              F0_0.rowid AS PROV_CUSTOMER_C__CUSTKEY
                                            FROM
                                              CUSTOMER AS F0_0
                                          ) AS F0_0
                                          CROSS JOIN (
                                            SELECT
                                              F0_0.O_ORDERKEY AS O_ORDERKEY,
                                              F0_0.O_CUSTKEY AS O_CUSTKEY,
                                              F0_0.O_ORDERDATE AS O_ORDERDATE,
                                              F0_0.rowid AS PROV_ORDERS_O__ORDERKEY
                                            FROM
                                              ORDERS AS F0_0
                                          ) AS F1_0
                                        )
                                    ) AS F0_0
                                    CROSS JOIN (
                                      SELECT
                                        F0_0.L_ORDERKEY AS L_ORDERKEY,
                                        F0_0.L_SUPPKEY AS L_SUPPKEY,
                                        F0_0.rowid AS PROV_LINEITEM_L__ORDERKEY
                                      FROM
                                        LINEITEM AS F0_0
                                    ) AS F1_0
                                  )
                              ) AS F0_0
                              CROSS JOIN (
                                SELECT
                                  F0_0.S_SUPPKEY AS S_SUPPKEY,
                                  F0_0.S_NATIONKEY AS S_NATIONKEY,
                                  F0_0.rowid AS PROV_SUPPLIER_S__SUPPKEY
                                FROM
                                  SUPPLIER AS F0_0
                              ) AS F1_0
                            )
                        ) AS F0_0
                        CROSS JOIN (
                          SELECT
                            F0_0.N_NATIONKEY AS N_NATIONKEY,
                            F0_0.N_NAME AS N_NAME,
                            F0_0.N_REGIONKEY AS N_REGIONKEY,
                            F0_0.rowid AS PROV_NATION_N__NATIONKEY
                          FROM
                            NATION AS F0_0
                        ) AS F1_0
                      )
                  ) AS F0_0
                  CROSS JOIN (
                    SELECT
                      F0_0.R_REGIONKEY AS R_REGIONKEY,
                      F0_0.R_NAME AS R_NAME,
                      F0_0.rowid AS PROV_REGION_R__REGIONKEY
                    FROM
                      REGION AS F0_0
                  ) AS F1_0
                )
            ) AS F0_0
          WHERE
            (
              (
                (
                  (
                    (
                      (
                        (
                          (
                            (F0_0.C_CUSTKEY = F0_0.O_CUSTKEY)
                            AND (F0_0.L_ORDERKEY = F0_0.O_ORDERKEY)
                          )
                          AND (F0_0.L_SUPPKEY = F0_0.S_SUPPKEY)
                        )
                        AND (F0_0.C_NATIONKEY = F0_0.S_NATIONKEY)
                      )
                      AND (F0_0.S_NATIONKEY = F0_0.N_NATIONKEY)
                    )
                    AND (F0_0.N_REGIONKEY = F0_0.R_REGIONKEY)
                  )
                  AND (F0_0.R_NAME = 'ASIA')
                )
                AND (F0_0.O_ORDERDATE >= '1994-01-01')
              )
              AND (F0_0.O_ORDERDATE < '1995-01-01')
            )
        ) AS F1_0 ON (
          (
            (F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")
            OR (
              (F0_0."GROUP_0" IS NULL)
              AND (F1_0."_P_SIDE_GROUP_0" IS NULL)
            )
          )
        )
      )
    ORDER BY
      REVENUE DESC
  ) AS F0_0
