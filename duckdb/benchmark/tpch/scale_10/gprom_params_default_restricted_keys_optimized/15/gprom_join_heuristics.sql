WITH
  temp_view_0 AS (
    /* + materialize */
    SELECT
      F0_0."GROUP_0" AS SUPPLIER_NO,
      F0_0."AGGR_0" AS TOTAL_REVENUE,
      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
    FROM
      (
        (
          SELECT
            SUM((F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT))) AS "AGGR_0",
            F0_0.L_SUPPKEY AS "GROUP_0"
          FROM
            (
              SELECT
                F0_0.L_SUPPKEY AS L_SUPPKEY,
                F0_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                F0_0.L_DISCOUNT AS L_DISCOUNT,
                F0_0.L_SHIPDATE AS L_SHIPDATE
              FROM
                LINEITEM AS F0_0
            ) AS F0_0
          WHERE
            (
              (F0_0.L_SHIPDATE >= CAST(('1996-01-01') AS DATE))
              AND (
                F0_0.L_SHIPDATE < (
                  CAST(('1996-01-01') AS DATE) + CAST(('3 month') AS INTERVAL)
                )
              )
            )
          GROUP BY
            F0_0.L_SUPPKEY
        ) AS F0_0
        JOIN (
          SELECT
            F0_0.L_SUPPKEY AS "_P_SIDE_GROUP_0",
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
          FROM
            (
              SELECT
                F0_0.L_SUPPKEY AS L_SUPPKEY,
                F0_0.L_SHIPDATE AS L_SHIPDATE,
                F0_0.rowid AS PROV_LINEITEM_L__ORDERKEY
              FROM
                LINEITEM AS F0_0
            ) AS F0_0
          WHERE
            (
              (F0_0.L_SHIPDATE >= CAST(('1996-01-01') AS DATE))
              AND (
                F0_0.L_SHIPDATE < (
                  CAST(('1996-01-01') AS DATE) + CAST(('3 month') AS INTERVAL)
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
  )
SELECT
  F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F0_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY",
  F0_0.S_SUPPKEY AS S_SUPPKEY,
  F0_0.S_NAME AS S_NAME,
  F0_0.S_ADDRESS AS S_ADDRESS,
  F0_0.S_PHONE AS S_PHONE
FROM
  (
    SELECT
      F0_0.S_SUPPKEY AS S_SUPPKEY,
      F0_0.S_NAME AS S_NAME,
      F0_0.S_ADDRESS AS S_ADDRESS,
      F0_0.S_PHONE AS S_PHONE,
      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F0_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY"
    FROM
      (
        SELECT
          F0_0.S_SUPPKEY AS S_SUPPKEY,
          F0_0.S_NAME AS S_NAME,
          F0_0.S_ADDRESS AS S_ADDRESS,
          F0_0.S_PHONE AS S_PHONE,
          F0_0.SUPPLIER_NO AS SUPPLIER_NO,
          F0_0.TOTAL_REVENUE AS TOTAL_REVENUE,
          F1_0."NESTING_EVAL_1" AS "NESTING_EVAL_1",
          F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
          F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
          F1_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY"
        FROM
          (
            SELECT
              F0_0.S_SUPPKEY AS S_SUPPKEY,
              F0_0.S_NAME AS S_NAME,
              F0_0.S_ADDRESS AS S_ADDRESS,
              F0_0.S_PHONE AS S_PHONE,
              F1_0.SUPPLIER_NO AS SUPPLIER_NO,
              F1_0.TOTAL_REVENUE AS TOTAL_REVENUE,
              F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
              F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
            FROM
              (
                (
                  SELECT
                    F0_0.S_SUPPKEY AS S_SUPPKEY,
                    F0_0.S_NAME AS S_NAME,
                    F0_0.S_ADDRESS AS S_ADDRESS,
                    F0_0.S_PHONE AS S_PHONE,
                    F0_0.rowid AS PROV_SUPPLIER_S__SUPPKEY
                  FROM
                    SUPPLIER AS F0_0
                ) AS F0_0
                CROSS JOIN (
                  SELECT
                    *
                  FROM
                    temp_view_0
                ) AS F1_0
              )
          ) AS F0_0,
          LATERAL (
            SELECT
              F0_1."AGGR_0" AS "NESTING_EVAL_1",
              F1_1."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY"
            FROM
              (
                (
                  SELECT
                    MAX(F0_1."AGGR_0") AS "AGGR_0"
                  FROM
                    (
                      SELECT
                        SUM((F0_1.L_EXTENDEDPRICE * (1 - F0_1.L_DISCOUNT))) AS "AGGR_0",
                        F0_1.L_SUPPKEY AS "GROUP_0"
                      FROM
                        (
                          SELECT
                            F0_1.L_SUPPKEY AS L_SUPPKEY,
                            F0_1.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                            F0_1.L_DISCOUNT AS L_DISCOUNT,
                            F0_1.L_SHIPDATE AS L_SHIPDATE
                          FROM
                            LINEITEM AS F0_1
                        ) AS F0_1
                      WHERE
                        (
                          (F0_1.L_SHIPDATE >= CAST(('1996-01-01') AS DATE))
                          AND (
                            F0_1.L_SHIPDATE < (
                              CAST(('1996-01-01') AS DATE) + CAST(('3 month') AS INTERVAL)
                            )
                          )
                        )
                      GROUP BY
                        F0_1.L_SUPPKEY
                    ) AS F0_1
                ) AS F0_1
                LEFT OUTER JOIN (
                  SELECT
                    F0_1.PROV_LINEITEM_L__ORDERKEY AS "PROV_LINEITEM_1_L__ORDERKEY"
                  FROM
                    (
                      SELECT
                        *
                      FROM
                        temp_view_0
                    ) AS F0_1
                ) AS F1_1 ON ((1 = 1))
              )
          ) AS F1_0
      ) AS F0_0
    WHERE
      (
        (F0_0.S_SUPPKEY = F0_0.SUPPLIER_NO)
        AND (F0_0.TOTAL_REVENUE = F0_0."NESTING_EVAL_1")
      )
    ORDER BY
      S_SUPPKEY ASC
  ) AS F0_0
