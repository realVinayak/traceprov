SELECT
  F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
  F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY
FROM
  (
    SELECT
      F0_0."GROUP_0" AS C_COUNT,
      F0_0."AGGR_0" AS CUSTDIST,
      F1_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY
    FROM
      (
        (
          SELECT
            COUNT(1) AS "AGGR_0",
            F0_0."AGGR_0" AS "GROUP_0"
          FROM
            (
              SELECT
                COUNT(F1_0.O_ORDERKEY) AS "AGGR_0",
                F0_0.C_CUSTKEY AS "GROUP_0"
              FROM
                (
                  (
                    SELECT
                      F0_0.C_CUSTKEY AS C_CUSTKEY
                    FROM
                      CUSTOMER AS F0_0
                  ) AS F0_0
                  LEFT OUTER JOIN (
                    SELECT
                      F0_0.O_ORDERKEY AS O_ORDERKEY,
                      F0_0.O_CUSTKEY AS O_CUSTKEY,
                      F0_0.O_COMMENT AS O_COMMENT
                    FROM
                      ORDERS AS F0_0
                  ) AS F1_0 ON (
                    (
                      (F0_0.C_CUSTKEY = F1_0.O_CUSTKEY)
                      AND (NOT ((F1_0.O_COMMENT LIKE '%special%requests%')))
                    )
                  )
                )
              GROUP BY
                F0_0.C_CUSTKEY
            ) AS F0_0
          GROUP BY
            F0_0."AGGR_0"
        ) AS F0_0
        JOIN (
          SELECT
            F0_0."AGGR_0" AS "_P_SIDE_GROUP_0",
            F1_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
            F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY
          FROM
            (
              (
                SELECT
                  COUNT(F1_0.O_ORDERKEY) AS "AGGR_0",
                  F0_0.C_CUSTKEY AS "GROUP_0"
                FROM
                  (
                    (
                      SELECT
                        F0_0.C_CUSTKEY AS C_CUSTKEY
                      FROM
                        CUSTOMER AS F0_0
                    ) AS F0_0
                    LEFT OUTER JOIN (
                      SELECT
                        F0_0.O_ORDERKEY AS O_ORDERKEY,
                        F0_0.O_CUSTKEY AS O_CUSTKEY,
                        F0_0.O_COMMENT AS O_COMMENT
                      FROM
                        ORDERS AS F0_0
                    ) AS F1_0 ON (
                      (
                        (F0_0.C_CUSTKEY = F1_0.O_CUSTKEY)
                        AND (NOT ((F1_0.O_COMMENT LIKE '%special%requests%')))
                      )
                    )
                  )
                GROUP BY
                  F0_0.C_CUSTKEY
              ) AS F0_0
              JOIN (
                SELECT
                  F0_0.C_CUSTKEY AS "_P_SIDE_GROUP_0",
                  F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                  F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY
                FROM
                  (
                    (
                      SELECT
                        F0_0.C_CUSTKEY AS C_CUSTKEY,
                        F0_0.rowid AS PROV_CUSTOMER_C__CUSTKEY
                      FROM
                        CUSTOMER AS F0_0
                    ) AS F0_0
                    LEFT OUTER JOIN (
                      SELECT
                        F0_0.O_CUSTKEY AS O_CUSTKEY,
                        F0_0.O_COMMENT AS O_COMMENT,
                        F0_0.rowid AS PROV_ORDERS_O__ORDERKEY
                      FROM
                        ORDERS AS F0_0
                    ) AS F1_0 ON (
                      (
                        (F0_0.C_CUSTKEY = F1_0.O_CUSTKEY)
                        AND (NOT ((F1_0.O_COMMENT LIKE '%special%requests%')))
                      )
                    )
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
      CUSTDIST DESC,
      C_COUNT DESC
  ) AS F0_0
