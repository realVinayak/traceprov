SELECT
  F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
  F0_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY"
FROM
  (
    SELECT
      F0_0."GROUP_0" AS CNTRYCODE,
      F1_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F1_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY"
    FROM
      (
        (
          SELECT
            SUBSTRING(F0_0.C_PHONE, 1, 2) AS "GROUP_0"
          FROM
            (
              SELECT
                F0_0.C_CUSTKEY AS C_CUSTKEY,
                F0_0.C_PHONE AS C_PHONE,
                F0_0.C_ACCTBAL AS C_ACCTBAL
              FROM
                CUSTOMER AS F0_0
            ) AS F0_0,
            LATERAL (
              SELECT
                AVG(F0_1.C_ACCTBAL) AS "NESTING_EVAL_1"
              FROM
                (
                  SELECT
                    F0_1.C_PHONE AS C_PHONE,
                    F0_1.C_ACCTBAL AS C_ACCTBAL
                  FROM
                    CUSTOMER AS F0_1
                ) AS F0_1
              WHERE
                (
                  (F0_1.C_ACCTBAL > 0.000000)
                  AND SUBSTRING(F0_1.C_PHONE, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17')
                )
            ) AS F1_0,
            LATERAL (
              SELECT
                (COUNT(1) > 0) AS "NESTING_EVAL_2"
              FROM
                (
                  SELECT
                    F0_1.O_CUSTKEY AS O_CUSTKEY
                  FROM
                    ORDERS AS F0_1
                ) AS F0_1
              WHERE
                (F0_1.O_CUSTKEY = F0_0.C_CUSTKEY)
            ) AS F2_0
          WHERE
            (
              (
                SUBSTRING(F0_0.C_PHONE, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17')
                AND (F0_0.C_ACCTBAL > F1_0."NESTING_EVAL_1")
              )
              AND (NOT (F2_0."NESTING_EVAL_2"))
            )
          GROUP BY
            SUBSTRING(F0_0.C_PHONE, 1, 2)
        ) AS F0_0
        JOIN (
          SELECT
            SUBSTRING(F0_0.C_PHONE, 1, 2) AS "_P_SIDE_GROUP_0",
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
                F0_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY"
              FROM
                (
                  SELECT
                    F0_0.C_CUSTKEY AS C_CUSTKEY,
                    F0_0.C_PHONE AS C_PHONE,
                    F0_0.C_ACCTBAL AS C_ACCTBAL,
                    F1_0."NESTING_EVAL_1" AS "NESTING_EVAL_1",
                    F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                    F1_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY"
                  FROM
                    (
                      SELECT
                        F0_0.C_CUSTKEY AS C_CUSTKEY,
                        F0_0.C_PHONE AS C_PHONE,
                        F0_0.C_ACCTBAL AS C_ACCTBAL,
                        F0_0.rowid AS PROV_CUSTOMER_C__CUSTKEY
                      FROM
                        CUSTOMER AS F0_0
                    ) AS F0_0,
                    LATERAL (
                      SELECT
                        F0_1."AGGR_0" AS "NESTING_EVAL_1",
                        F1_1."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY"
                      FROM
                        (
                          (
                            SELECT
                              AVG(F0_1.C_ACCTBAL) AS "AGGR_0"
                            FROM
                              (
                                SELECT
                                  F0_1.C_PHONE AS C_PHONE,
                                  F0_1.C_ACCTBAL AS C_ACCTBAL
                                FROM
                                  CUSTOMER AS F0_1
                              ) AS F0_1
                            WHERE
                              (
                                (F0_1.C_ACCTBAL > 0.000000)
                                AND SUBSTRING(F0_1.C_PHONE, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17')
                              )
                          ) AS F0_1
                          LEFT OUTER JOIN (
                            SELECT
                              F0_1.C_PHONE AS C_PHONE,
                              F0_1.C_ACCTBAL AS C_ACCTBAL,
                              F0_1."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY"
                            FROM
                              (
                                SELECT
                                  F0_1.C_PHONE AS C_PHONE,
                                  F0_1.C_ACCTBAL AS C_ACCTBAL,
                                  F0_1.rowid AS PROV_CUSTOMER_1_C__CUSTKEY
                                FROM
                                  CUSTOMER AS F0_1
                              ) AS F0_1
                            WHERE
                              (
                                (F0_1.C_ACCTBAL > 0.000000)
                                AND SUBSTRING(F0_1.C_PHONE, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17')
                              )
                          ) AS F1_1 ON ((1 = 1))
                        )
                    ) AS F1_0
                ) AS F0_0,
                LATERAL (
                  SELECT
                    (F0_1."AGGR_0" > 0) AS "NESTING_EVAL_2"
                  FROM
                    (
                      (
                        SELECT
                          COUNT(1) AS "AGGR_0"
                        FROM
                          (
                            SELECT
                              F0_1.O_CUSTKEY AS O_CUSTKEY
                            FROM
                              ORDERS AS F0_1
                          ) AS F0_1
                        WHERE
                          (F0_1.O_CUSTKEY = F0_0.C_CUSTKEY)
                      ) AS F0_1
                      LEFT OUTER JOIN (
                        SELECT
                          F0_1.O_CUSTKEY AS O_CUSTKEY
                        FROM
                          (
                            SELECT
                              F0_1.O_CUSTKEY AS O_CUSTKEY
                            FROM
                              ORDERS AS F0_1
                          ) AS F0_1
                        WHERE
                          (F0_1.O_CUSTKEY = F0_0.C_CUSTKEY)
                      ) AS F1_1 ON ((1 = 1))
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
      CNTRYCODE ASC
  ) AS F0_0
