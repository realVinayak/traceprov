SELECT
  F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
  F0_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
  F0_0.CNTRYCODE AS CNTRYCODE
FROM
  (
    SELECT
      SUBSTRING(F0_0.C_PHONE, 1, 2) AS CNTRYCODE,
      F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F0_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY"
    FROM
      (
        SELECT
          F0_0.C_PHONE AS C_PHONE,
          F0_0.C_ACCTBAL AS C_ACCTBAL,
          F0_0."NESTING_EVAL_1" AS "NESTING_EVAL_1",
          F1_0."NESTING_EVAL_2" AS "NESTING_EVAL_2",
          F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
          F0_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
          GREATEST(
            F0_0.left__SETPROV_DUP_COUNT,
            F1_0.right__SETPROV_DUP_COUNT
          ) AS _SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              F0_0.C_CUSTKEY AS C_CUSTKEY,
              F0_0.C_PHONE AS C_PHONE,
              F0_0.C_ACCTBAL AS C_ACCTBAL,
              F1_0."NESTING_EVAL_1" AS "NESTING_EVAL_1",
              F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
              F1_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
              GREATEST(
                F0_0.left__SETPROV_DUP_COUNT,
                F1_0.right__SETPROV_DUP_COUNT
              ) AS left__SETPROV_DUP_COUNT
            FROM
              (
                SELECT
                  F0_0.C_CUSTKEY AS C_CUSTKEY,
                  F0_0.C_PHONE AS C_PHONE,
                  F0_0.C_ACCTBAL AS C_ACCTBAL,
                  F0_0.rowid AS PROV_CUSTOMER_C__CUSTKEY,
                  1 AS left__SETPROV_DUP_COUNT
                FROM
                  CUSTOMER AS F0_0
              ) AS F0_0,
              LATERAL (
                SELECT
                  F0_1."AGGR_0" AS "NESTING_EVAL_1",
                  F0_1."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
                  ROW_NUMBER() OVER () AS right__SETPROV_DUP_COUNT
                FROM
                  (
                    SELECT
                      F0_1.C_ACCTBAL AS C_ACCTBAL,
                      F0_1."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
                      F0_1._RESULT_TID AS _RESULT_TID,
                      AVG(F0_1.C_ACCTBAL) OVER () AS "AGGR_0",
                      COUNT(1) OVER () AS __DUMMY_CNT
                    FROM
                      (
                        (
                          SELECT
                            F0_1.C_ACCTBAL AS C_ACCTBAL,
                            F0_1."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
                            F0_1._RESULT_TID AS _RESULT_TID
                          FROM
                            (
                              SELECT
                                F0_1.C_PHONE AS C_PHONE,
                                F0_1.C_ACCTBAL AS C_ACCTBAL,
                                F0_1.rowid AS PROV_CUSTOMER_1_C__CUSTKEY,
                                F0_1.rowid AS _RESULT_TID
                              FROM
                                CUSTOMER AS F0_1
                            ) AS F0_1
                          WHERE
                            (
                              (F0_1.C_ACCTBAL > 0.000000)
                              AND SUBSTRING(F0_1.C_PHONE, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17')
                            )
                          UNION ALL
                          (
                            SELECT
                              NULL AS C_ACCTBAL,
                              NULL AS "PROV_CUSTOMER_1_C__CUSTKEY",
                              -1 AS _RESULT_TID
                          )
                        )
                      ) AS F0_1
                  ) AS F0_1
                WHERE
                  (
                    (F0_1.__DUMMY_CNT = 1)
                    OR (F0_1._RESULT_TID <> -1)
                  )
              ) AS F1_0
          ) AS F0_0,
          LATERAL (
            SELECT
              (F0_1."AGGR_0" > 0) AS "NESTING_EVAL_2",
              ROW_NUMBER() OVER () AS right__SETPROV_DUP_COUNT
            FROM
              (
                SELECT
                  F0_1._RESULT_TID AS _RESULT_TID,
                  COUNT(1) OVER () AS "AGGR_0"
                FROM
                  (
                    (
                      SELECT
                        F0_1._RESULT_TID AS _RESULT_TID
                      FROM
                        (
                          SELECT
                            F0_1.O_CUSTKEY AS O_CUSTKEY,
                            F0_1.rowid AS _RESULT_TID
                          FROM
                            ORDERS AS F0_1
                        ) AS F0_1
                      WHERE
                        (F0_1.O_CUSTKEY = F0_0.C_CUSTKEY)
                      UNION ALL
                      (
                        SELECT
                          -1 AS _RESULT_TID
                      )
                    )
                  ) AS F0_1
              ) AS F0_1
            WHERE
              (
                (F0_1."AGGR_0" = 0)
                OR (F0_1._RESULT_TID <> -1)
              )
          ) AS F1_0
      ) AS F0_0
    WHERE
      (
        (
          SUBSTRING(F0_0.C_PHONE, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17')
          AND (F0_0.C_ACCTBAL > F0_0."NESTING_EVAL_1")
        )
        AND (NOT (F0_0."NESTING_EVAL_2"))
      )
    ORDER BY
      CNTRYCODE ASC
  ) AS F0_0
