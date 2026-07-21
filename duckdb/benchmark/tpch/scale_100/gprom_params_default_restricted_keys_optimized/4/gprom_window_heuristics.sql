SELECT
  F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY
FROM
  (
    SELECT
      F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
    FROM
      (
        SELECT
          F0_0.O_ORDERDATE AS O_ORDERDATE,
          F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
          F1_0."NESTING_EVAL_1" AS "NESTING_EVAL_1",
          F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
          F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
          GREATEST(
            F0_0.left__SETPROV_DUP_COUNT,
            F1_0.right__SETPROV_DUP_COUNT
          ) AS _SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              F0_0.O_ORDERKEY AS O_ORDERKEY,
              F0_0.O_ORDERDATE AS O_ORDERDATE,
              F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
              F0_0.rowid AS PROV_ORDERS_O__ORDERKEY,
              1 AS left__SETPROV_DUP_COUNT
            FROM
              ORDERS AS F0_0
          ) AS F0_0,
          LATERAL (
            SELECT
              (F0_1."AGGR_0" > 0) AS "NESTING_EVAL_1",
              F0_1.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
              ROW_NUMBER() OVER () AS right__SETPROV_DUP_COUNT
            FROM
              (
                SELECT
                  F0_1.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                  F0_1._RESULT_TID AS _RESULT_TID,
                  COUNT(1) OVER () AS "AGGR_0"
                FROM
                  (
                    (
                      SELECT
                        F0_1.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                        F0_1._RESULT_TID AS _RESULT_TID
                      FROM
                        (
                          SELECT
                            F0_1.L_ORDERKEY AS L_ORDERKEY,
                            F0_1.L_COMMITDATE AS L_COMMITDATE,
                            F0_1.L_RECEIPTDATE AS L_RECEIPTDATE,
                            F0_1.rowid AS PROV_LINEITEM_L__ORDERKEY,
                            F0_1.rowid AS _RESULT_TID
                          FROM
                            LINEITEM AS F0_1
                        ) AS F0_1
                      WHERE
                        (
                          (F0_1.L_ORDERKEY = F0_0.O_ORDERKEY)
                          AND (F0_1.L_COMMITDATE < F0_1.L_RECEIPTDATE)
                        )
                      UNION ALL
                      (
                        SELECT
                          NULL AS PROV_LINEITEM_L__ORDERKEY,
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
          (F0_0.O_ORDERDATE >= '1993-07-01')
          AND (F0_0.O_ORDERDATE < '1993-10-01')
        )
        AND F0_0."NESTING_EVAL_1"
      )
    ORDER BY
      O_ORDERPRIORITY ASC
  ) AS F0_0
