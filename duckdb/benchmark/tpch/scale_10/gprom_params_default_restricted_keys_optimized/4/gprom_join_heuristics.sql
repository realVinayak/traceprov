SELECT
  F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY
FROM
  (
    SELECT
      F0_0."GROUP_0" AS O_ORDERPRIORITY,
      F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
    FROM
      (
        (
          SELECT
            F0_0.O_ORDERPRIORITY AS "GROUP_0"
          FROM
            (
              (
                SELECT
                  F0_0.O_ORDERKEY AS O_ORDERKEY,
                  F0_0.O_ORDERDATE AS O_ORDERDATE,
                  F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY
                FROM
                  ORDERS AS F0_0
              ) AS F0_0
              JOIN (
                SELECT
                  F0_0.L_ORDERKEY AS L_ORDERKEY
                FROM
                  (
                    SELECT
                      F0_0.L_ORDERKEY AS L_ORDERKEY,
                      F0_0.L_COMMITDATE AS L_COMMITDATE,
                      F0_0.L_RECEIPTDATE AS L_RECEIPTDATE
                    FROM
                      LINEITEM AS F0_0
                  ) AS F0_0
                WHERE
                  (F0_0.L_COMMITDATE < F0_0.L_RECEIPTDATE)
                GROUP BY
                  F0_0.L_ORDERKEY
              ) AS F1_0 ON ((F0_0.O_ORDERKEY = F1_0.L_ORDERKEY))
            )
          WHERE
            (
              (F0_0.O_ORDERDATE >= '1993-07-01')
              AND (F0_0.O_ORDERDATE < '1993-10-01')
            )
          GROUP BY
            F0_0.O_ORDERPRIORITY
        ) AS F0_0
        JOIN (
          SELECT
            F0_0.O_ORDERPRIORITY AS "_P_SIDE_GROUP_0",
            F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
          FROM
            (
              SELECT
                F0_0.O_ORDERDATE AS O_ORDERDATE,
                F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
                F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
              FROM
                (
                  (
                    SELECT
                      F0_0.O_ORDERKEY AS O_ORDERKEY,
                      F0_0.O_ORDERDATE AS O_ORDERDATE,
                      F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
                      F0_0.rowid AS PROV_ORDERS_O__ORDERKEY
                    FROM
                      ORDERS AS F0_0
                  ) AS F0_0
                  JOIN (
                    SELECT
                      F0_0."GROUP_0" AS L_ORDERKEY,
                      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
                    FROM
                      (
                        (
                          SELECT
                            F0_0.L_ORDERKEY AS "GROUP_0"
                          FROM
                            (
                              SELECT
                                F0_0.L_ORDERKEY AS L_ORDERKEY,
                                F0_0.L_COMMITDATE AS L_COMMITDATE,
                                F0_0.L_RECEIPTDATE AS L_RECEIPTDATE
                              FROM
                                LINEITEM AS F0_0
                            ) AS F0_0
                          WHERE
                            (F0_0.L_COMMITDATE < F0_0.L_RECEIPTDATE)
                          GROUP BY
                            F0_0.L_ORDERKEY
                        ) AS F0_0
                        JOIN (
                          SELECT
                            F0_0.L_ORDERKEY AS "_P_SIDE_GROUP_0",
                            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
                          FROM
                            (
                              SELECT
                                F0_0.L_ORDERKEY AS L_ORDERKEY,
                                F0_0.L_COMMITDATE AS L_COMMITDATE,
                                F0_0.L_RECEIPTDATE AS L_RECEIPTDATE,
                                F0_0.rowid AS PROV_LINEITEM_L__ORDERKEY
                              FROM
                                LINEITEM AS F0_0
                            ) AS F0_0
                          WHERE
                            (F0_0.L_COMMITDATE < F0_0.L_RECEIPTDATE)
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
                  ) AS F1_0 ON ((F0_0.O_ORDERKEY = F1_0.L_ORDERKEY))
                )
            ) AS F0_0
          WHERE
            (
              (F0_0.O_ORDERDATE >= '1993-07-01')
              AND (F0_0.O_ORDERDATE < '1993-10-01')
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
      O_ORDERPRIORITY ASC
  ) AS F0_0
