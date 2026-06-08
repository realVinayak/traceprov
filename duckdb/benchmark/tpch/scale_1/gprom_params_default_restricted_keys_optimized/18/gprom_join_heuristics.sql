SELECT
  F1_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
  F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
  F1_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY",
  F0_0.C_NAME AS C_NAME,
  F0_0.C_CUSTKEY AS C_CUSTKEY,
  F0_0.O_ORDERKEY AS O_ORDERKEY,
  F0_0.O_ORDERDATE AS O_ORDERDATE,
  F0_0.O_TOTALPRICE AS O_TOTALPRICE
FROM
  (
    (
      SELECT
        F0_0.C_NAME AS C_NAME,
        F0_0.C_CUSTKEY AS C_CUSTKEY,
        F2_0.O_ORDERKEY AS O_ORDERKEY,
        F2_0.O_ORDERDATE AS O_ORDERDATE,
        F2_0.O_TOTALPRICE AS O_TOTALPRICE,
        SUM(F1_0.L_QUANTITY) AS "SUM(L_QUANTITY)"
      FROM
        (
          (
            (
              SELECT
                F0_0.C_CUSTKEY AS C_CUSTKEY,
                F0_0.C_NAME AS C_NAME
              FROM
                CUSTOMER AS F0_0
            ) AS F0_0
            CROSS JOIN (
              SELECT
                F0_0.L_ORDERKEY AS L_ORDERKEY,
                F0_0.L_QUANTITY AS L_QUANTITY
              FROM
                LINEITEM AS F0_0
            ) AS F1_0
          )
          CROSS JOIN (
            (
              SELECT
                F0_0.O_ORDERKEY AS O_ORDERKEY,
                F0_0.O_CUSTKEY AS O_CUSTKEY,
                F0_0.O_TOTALPRICE AS O_TOTALPRICE,
                F0_0.O_ORDERDATE AS O_ORDERDATE
              FROM
                ORDERS AS F0_0
            ) AS F2_0
            JOIN (
              SELECT
                F0_0.L_ORDERKEY AS INNER_L_ORDERKEY
              FROM
                LINEITEM AS F0_0
              GROUP BY
                F0_0.L_ORDERKEY
              HAVING
                (SUM(F0_0.L_QUANTITY) > 300)
            ) AS F3_0 ON ((F3_0.INNER_L_ORDERKEY = F0_0.C_CUSTKEY))
          )
        )
      WHERE
        (
          (F0_0.C_CUSTKEY = F2_0.O_CUSTKEY)
          AND (F2_0.O_ORDERKEY = F1_0.L_ORDERKEY)
        )
      GROUP BY
        F0_0.C_NAME,
        F0_0.C_CUSTKEY,
        F2_0.O_ORDERKEY,
        F2_0.O_ORDERDATE,
        F2_0.O_TOTALPRICE
      ORDER BY
        O_TOTALPRICE DESC,
        O_ORDERDATE ASC
      LIMIT
        100
    ) AS F0_0
    JOIN (
      SELECT
        F0_0."GROUP_0" AS C_NAME,
        F0_0."GROUP_1" AS C_CUSTKEY,
        F0_0."GROUP_2" AS O_ORDERKEY,
        F0_0."GROUP_3" AS O_ORDERDATE,
        F0_0."GROUP_4" AS O_TOTALPRICE,
        F0_0."AGGR_0" AS "SUM(L_QUANTITY)",
        F1_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
        F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
        F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
        F1_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY"
      FROM
        (
          (
            SELECT
              SUM(F1_0.L_QUANTITY) AS "AGGR_0",
              F0_0.C_NAME AS "GROUP_0",
              F0_0.C_CUSTKEY AS "GROUP_1",
              F2_0.O_ORDERKEY AS "GROUP_2",
              F2_0.O_ORDERDATE AS "GROUP_3",
              F2_0.O_TOTALPRICE AS "GROUP_4"
            FROM
              (
                (
                  (
                    SELECT
                      F0_0.C_CUSTKEY AS C_CUSTKEY,
                      F0_0.C_NAME AS C_NAME
                    FROM
                      CUSTOMER AS F0_0
                  ) AS F0_0
                  CROSS JOIN (
                    SELECT
                      F0_0.L_ORDERKEY AS L_ORDERKEY,
                      F0_0.L_QUANTITY AS L_QUANTITY
                    FROM
                      LINEITEM AS F0_0
                  ) AS F1_0
                )
                CROSS JOIN (
                  (
                    SELECT
                      F0_0.O_ORDERKEY AS O_ORDERKEY,
                      F0_0.O_CUSTKEY AS O_CUSTKEY,
                      F0_0.O_TOTALPRICE AS O_TOTALPRICE,
                      F0_0.O_ORDERDATE AS O_ORDERDATE
                    FROM
                      ORDERS AS F0_0
                  ) AS F2_0
                  JOIN (
                    SELECT
                      F0_0.L_ORDERKEY AS INNER_L_ORDERKEY
                    FROM
                      LINEITEM AS F0_0
                    GROUP BY
                      F0_0.L_ORDERKEY
                    HAVING
                      (SUM(F0_0.L_QUANTITY) > 300)
                  ) AS F3_0 ON ((F3_0.INNER_L_ORDERKEY = F0_0.C_CUSTKEY))
                )
              )
            WHERE
              (
                (F0_0.C_CUSTKEY = F2_0.O_CUSTKEY)
                AND (F2_0.O_ORDERKEY = F1_0.L_ORDERKEY)
              )
            GROUP BY
              F0_0.C_NAME,
              F0_0.C_CUSTKEY,
              F2_0.O_ORDERKEY,
              F2_0.O_ORDERDATE,
              F2_0.O_TOTALPRICE
          ) AS F0_0
          JOIN (
            SELECT
              F0_0.C_NAME AS "_P_SIDE_GROUP_0",
              F0_0.C_CUSTKEY AS "_P_SIDE_GROUP_1",
              F0_0.O_ORDERKEY AS "_P_SIDE_GROUP_2",
              F0_0.O_ORDERDATE AS "_P_SIDE_GROUP_3",
              F0_0.O_TOTALPRICE AS "_P_SIDE_GROUP_4",
              F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
              F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
              F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
              F0_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY"
            FROM
              (
                SELECT
                  F0_0.C_CUSTKEY AS C_CUSTKEY,
                  F0_0.C_NAME AS C_NAME,
                  F0_0.L_ORDERKEY AS L_ORDERKEY,
                  F1_0.O_ORDERKEY AS O_ORDERKEY,
                  F1_0.O_CUSTKEY AS O_CUSTKEY,
                  F1_0.O_TOTALPRICE AS O_TOTALPRICE,
                  F1_0.O_ORDERDATE AS O_ORDERDATE,
                  F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                  F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                  F1_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY"
                FROM
                  (
                    (
                      SELECT
                        F0_0.C_CUSTKEY AS C_CUSTKEY,
                        F0_0.C_NAME AS C_NAME,
                        F1_0.L_ORDERKEY AS L_ORDERKEY,
                        F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                        F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
                      FROM
                        (
                          (
                            SELECT
                              F0_0.C_CUSTKEY AS C_CUSTKEY,
                              F0_0.C_NAME AS C_NAME,
                              F0_0.rowid AS PROV_CUSTOMER_C__CUSTKEY
                            FROM
                              CUSTOMER AS F0_0
                          ) AS F0_0
                          CROSS JOIN (
                            SELECT
                              F0_0.L_ORDERKEY AS L_ORDERKEY,
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
                        F0_0.O_TOTALPRICE AS O_TOTALPRICE,
                        F0_0.O_ORDERDATE AS O_ORDERDATE,
                        F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                        F1_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY"
                      FROM
                        (
                          (
                            SELECT
                              F0_0.O_ORDERKEY AS O_ORDERKEY,
                              F0_0.O_CUSTKEY AS O_CUSTKEY,
                              F0_0.O_TOTALPRICE AS O_TOTALPRICE,
                              F0_0.O_ORDERDATE AS O_ORDERDATE,
                              F0_0.rowid AS PROV_ORDERS_O__ORDERKEY
                            FROM
                              ORDERS AS F0_0
                          ) AS F0_0
                          JOIN (
                            SELECT
                              F0_0."GROUP_0" AS INNER_L_ORDERKEY,
                              F0_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY"
                            FROM
                              (
                                SELECT
                                  F0_0."AGGR_0" AS "AGGR_0",
                                  F0_0."GROUP_0" AS "GROUP_0",
                                  F1_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY"
                                FROM
                                  (
                                    (
                                      SELECT
                                        SUM(F0_0.L_QUANTITY) AS "AGGR_0",
                                        F0_0.L_ORDERKEY AS "GROUP_0"
                                      FROM
                                        LINEITEM AS F0_0
                                      GROUP BY
                                        F0_0.L_ORDERKEY
                                    ) AS F0_0
                                    JOIN (
                                      SELECT
                                        F0_0.L_ORDERKEY AS "_P_SIDE_GROUP_0",
                                        F0_0.rowid AS PROV_LINEITEM_1_L__ORDERKEY
                                      FROM
                                        LINEITEM AS F0_0
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
                              ) AS F0_0
                            WHERE
                              (F0_0."AGGR_0" > 300)
                          ) AS F1_0 ON ((F1_0.INNER_L_ORDERKEY = F0_0.O_ORDERKEY))
                        )
                    ) AS F1_0
                  )
              ) AS F0_0
            WHERE
              (
                (F0_0.C_CUSTKEY = F0_0.O_CUSTKEY)
                AND (F0_0.O_ORDERKEY = F0_0.L_ORDERKEY)
              )
          ) AS F1_0 ON (
            (
              (
                (F0_0."GROUP_4" = F1_0."_P_SIDE_GROUP_4")
                OR (
                  (F0_0."GROUP_4" IS NULL)
                  AND (F1_0."_P_SIDE_GROUP_4" IS NULL)
                )
              )
              AND (
                (
                  (F0_0."GROUP_3" = F1_0."_P_SIDE_GROUP_3")
                  OR (
                    (F0_0."GROUP_3" IS NULL)
                    AND (F1_0."_P_SIDE_GROUP_3" IS NULL)
                  )
                )
                AND (
                  (
                    (F0_0."GROUP_2" = F1_0."_P_SIDE_GROUP_2")
                    OR (
                      (F0_0."GROUP_2" IS NULL)
                      AND (F1_0."_P_SIDE_GROUP_2" IS NULL)
                    )
                  )
                  AND (
                    (
                      (F0_0."GROUP_1" = F1_0."_P_SIDE_GROUP_1")
                      OR (
                        (F0_0."GROUP_1" IS NULL)
                        AND (F1_0."_P_SIDE_GROUP_1" IS NULL)
                      )
                    )
                    AND (
                      (F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")
                      OR (
                        (F0_0."GROUP_0" IS NULL)
                        AND (F1_0."_P_SIDE_GROUP_0" IS NULL)
                      )
                    )
                  )
                )
              )
            )
          )
        )
      ORDER BY
        O_TOTALPRICE DESC,
        O_ORDERDATE ASC
    ) AS F1_0 ON (
      (
        (
          (
            (
              (
                (F0_0.C_NAME = F1_0.C_NAME)
                AND (F0_0.C_CUSTKEY = F1_0.C_CUSTKEY)
              )
              AND (F0_0.O_ORDERKEY = F1_0.O_ORDERKEY)
            )
            AND (F0_0.O_ORDERDATE = F1_0.O_ORDERDATE)
          )
          AND (F0_0.O_TOTALPRICE = F1_0.O_TOTALPRICE)
        )
        AND (F0_0."SUM(L_QUANTITY)" = F1_0."SUM(L_QUANTITY)")
      )
    )
  )
