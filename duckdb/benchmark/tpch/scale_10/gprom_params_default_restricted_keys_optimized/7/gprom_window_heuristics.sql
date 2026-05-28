SELECT
  F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
  F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
  F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
  F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
  F0_0.SUPP_NATION AS SUPP_NATION,
  F0_0.CUST_NATION AS CUST_NATION,
  F0_0.L_YEAR AS L_YEAR
FROM
  (
    SELECT
      F0_0.N_NAME AS SUPP_NATION,
      F0_0."N_NAME1" AS CUST_NATION,
      DATE_PART('YEAR', CAST((F0_0.L_SHIPDATE) AS DATE)) AS L_YEAR,
      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
      F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY"
    FROM
      (
        SELECT
          F0_0.S_SUPPKEY AS S_SUPPKEY,
          F0_0.S_NATIONKEY AS S_NATIONKEY,
          F0_0.L_ORDERKEY AS L_ORDERKEY,
          F0_0.L_SUPPKEY AS L_SUPPKEY,
          F0_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
          F0_0.L_DISCOUNT AS L_DISCOUNT,
          F0_0.L_SHIPDATE AS L_SHIPDATE,
          F0_0.O_ORDERKEY AS O_ORDERKEY,
          F0_0.O_CUSTKEY AS O_CUSTKEY,
          F0_0.C_CUSTKEY AS C_CUSTKEY,
          F0_0.C_NATIONKEY AS C_NATIONKEY,
          F0_0.N_NATIONKEY AS N_NATIONKEY,
          F0_0.N_NAME AS N_NAME,
          F1_0.N_NATIONKEY AS "N_NATIONKEY1",
          F1_0.N_NAME AS "N_NAME1",
          F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
          F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
          F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
          F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
          F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
          F1_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY"
        FROM
          (
            (
              SELECT
                F0_0.S_SUPPKEY AS S_SUPPKEY,
                F0_0.S_NATIONKEY AS S_NATIONKEY,
                F0_0.L_ORDERKEY AS L_ORDERKEY,
                F0_0.L_SUPPKEY AS L_SUPPKEY,
                F0_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                F0_0.L_DISCOUNT AS L_DISCOUNT,
                F0_0.L_SHIPDATE AS L_SHIPDATE,
                F0_0.O_ORDERKEY AS O_ORDERKEY,
                F0_0.O_CUSTKEY AS O_CUSTKEY,
                F0_0.C_CUSTKEY AS C_CUSTKEY,
                F0_0.C_NATIONKEY AS C_NATIONKEY,
                F1_0.N_NATIONKEY AS N_NATIONKEY,
                F1_0.N_NAME AS N_NAME,
                F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY
              FROM
                (
                  (
                    SELECT
                      F0_0.S_SUPPKEY AS S_SUPPKEY,
                      F0_0.S_NATIONKEY AS S_NATIONKEY,
                      F0_0.L_ORDERKEY AS L_ORDERKEY,
                      F0_0.L_SUPPKEY AS L_SUPPKEY,
                      F0_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                      F0_0.L_DISCOUNT AS L_DISCOUNT,
                      F0_0.L_SHIPDATE AS L_SHIPDATE,
                      F0_0.O_ORDERKEY AS O_ORDERKEY,
                      F0_0.O_CUSTKEY AS O_CUSTKEY,
                      F1_0.C_CUSTKEY AS C_CUSTKEY,
                      F1_0.C_NATIONKEY AS C_NATIONKEY,
                      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                      F1_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY
                    FROM
                      (
                        (
                          SELECT
                            F0_0.S_SUPPKEY AS S_SUPPKEY,
                            F0_0.S_NATIONKEY AS S_NATIONKEY,
                            F0_0.L_ORDERKEY AS L_ORDERKEY,
                            F0_0.L_SUPPKEY AS L_SUPPKEY,
                            F0_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                            F0_0.L_DISCOUNT AS L_DISCOUNT,
                            F0_0.L_SHIPDATE AS L_SHIPDATE,
                            F1_0.O_ORDERKEY AS O_ORDERKEY,
                            F1_0.O_CUSTKEY AS O_CUSTKEY,
                            F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                            F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY
                          FROM
                            (
                              (
                                SELECT
                                  F0_0.S_SUPPKEY AS S_SUPPKEY,
                                  F0_0.S_NATIONKEY AS S_NATIONKEY,
                                  F1_0.L_ORDERKEY AS L_ORDERKEY,
                                  F1_0.L_SUPPKEY AS L_SUPPKEY,
                                  F1_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                                  F1_0.L_DISCOUNT AS L_DISCOUNT,
                                  F1_0.L_SHIPDATE AS L_SHIPDATE,
                                  F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                                  F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
                                FROM
                                  (
                                    (
                                      SELECT
                                        F0_0.S_SUPPKEY AS S_SUPPKEY,
                                        F0_0.S_NATIONKEY AS S_NATIONKEY,
                                        F0_0.rowid AS PROV_SUPPLIER_S__SUPPKEY
                                      FROM
                                        SUPPLIER AS F0_0
                                    ) AS F0_0
                                    CROSS JOIN (
                                      SELECT
                                        F0_0.L_ORDERKEY AS L_ORDERKEY,
                                        F0_0.L_SUPPKEY AS L_SUPPKEY,
                                        F0_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                                        F0_0.L_DISCOUNT AS L_DISCOUNT,
                                        F0_0.L_SHIPDATE AS L_SHIPDATE,
                                        F0_0.rowid AS PROV_LINEITEM_L__ORDERKEY
                                      FROM
                                        LINEITEM AS F0_0
                                    ) AS F1_0
                                  )
                              ) AS F0_0
                              CROSS JOIN (
                                SELECT
                                  F0_0.O_ORDERKEY AS O_ORDERKEY,
                                  F0_0.O_CUSTKEY AS O_CUSTKEY,
                                  F0_0.rowid AS PROV_ORDERS_O__ORDERKEY
                                FROM
                                  ORDERS AS F0_0
                              ) AS F1_0
                            )
                        ) AS F0_0
                        CROSS JOIN (
                          SELECT
                            F0_0.C_CUSTKEY AS C_CUSTKEY,
                            F0_0.C_NATIONKEY AS C_NATIONKEY,
                            F0_0.rowid AS PROV_CUSTOMER_C__CUSTKEY
                          FROM
                            CUSTOMER AS F0_0
                        ) AS F1_0
                      )
                  ) AS F0_0
                  CROSS JOIN (
                    SELECT
                      F0_0.N_NATIONKEY AS N_NATIONKEY,
                      F0_0.N_NAME AS N_NAME,
                      F0_0.rowid AS PROV_NATION_N__NATIONKEY
                    FROM
                      NATION AS F0_0
                  ) AS F1_0
                )
            ) AS F0_0
            CROSS JOIN (
              SELECT
                F0_0.N_NATIONKEY AS N_NATIONKEY,
                F0_0.N_NAME AS N_NAME,
                F0_0.rowid AS PROV_NATION_1_N__NATIONKEY
              FROM
                NATION AS F0_0
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
                    (F0_0.S_SUPPKEY = F0_0.L_SUPPKEY)
                    AND (F0_0.O_ORDERKEY = F0_0.L_ORDERKEY)
                  )
                  AND (F0_0.C_CUSTKEY = F0_0.O_CUSTKEY)
                )
                AND (F0_0.S_NATIONKEY = F0_0.N_NATIONKEY)
              )
              AND (F0_0.C_NATIONKEY = F0_0."N_NATIONKEY1")
            )
            AND (
              (
                (F0_0.N_NAME = 'FRANCE')
                AND (F0_0."N_NAME1" = 'GERMANY')
              )
              OR (
                (F0_0.N_NAME = 'GERMANY')
                AND (F0_0."N_NAME1" = 'FRANCE')
              )
            )
          )
          AND (F0_0.L_SHIPDATE >= '1995-01-01')
        )
        AND (F0_0.L_SHIPDATE <= '1996-12-31')
      )
    ORDER BY
      SUPP_NATION ASC,
      CUST_NATION ASC,
      L_YEAR ASC
  ) AS F0_0
