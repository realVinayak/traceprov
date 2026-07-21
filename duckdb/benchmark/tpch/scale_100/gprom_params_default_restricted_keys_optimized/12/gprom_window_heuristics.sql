SELECT
  F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F0_0.L_SHIPMODE AS L_SHIPMODE
FROM
  (
    SELECT
      F0_0.L_SHIPMODE AS L_SHIPMODE,
      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
    FROM
      (
        SELECT
          F0_0.O_ORDERKEY AS O_ORDERKEY,
          F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
          F1_0.L_ORDERKEY AS L_ORDERKEY,
          F1_0.L_SHIPDATE AS L_SHIPDATE,
          F1_0.L_COMMITDATE AS L_COMMITDATE,
          F1_0.L_RECEIPTDATE AS L_RECEIPTDATE,
          F1_0.L_SHIPMODE AS L_SHIPMODE,
          F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
          F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
        FROM
          (
            (
              SELECT
                F0_0.O_ORDERKEY AS O_ORDERKEY,
                F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
                F0_0.rowid AS PROV_ORDERS_O__ORDERKEY
              FROM
                ORDERS AS F0_0
            ) AS F0_0
            CROSS JOIN (
              SELECT
                F0_0.L_ORDERKEY AS L_ORDERKEY,
                F0_0.L_SHIPDATE AS L_SHIPDATE,
                F0_0.L_COMMITDATE AS L_COMMITDATE,
                F0_0.L_RECEIPTDATE AS L_RECEIPTDATE,
                F0_0.L_SHIPMODE AS L_SHIPMODE,
                F0_0.rowid AS PROV_LINEITEM_L__ORDERKEY
              FROM
                LINEITEM AS F0_0
            ) AS F1_0
          )
      ) AS F0_0
    WHERE
      (
        (
          (
            (
              (
                (F0_0.O_ORDERKEY = F0_0.L_ORDERKEY)
                AND F0_0.L_SHIPMODE IN ('MAIL', 'SHIP')
              )
              AND (F0_0.L_COMMITDATE < F0_0.L_RECEIPTDATE)
            )
            AND (F0_0.L_SHIPDATE < F0_0.L_COMMITDATE)
          )
          AND (F0_0.L_RECEIPTDATE >= '1994-01-01')
        )
        AND (F0_0.L_RECEIPTDATE < '1995-01-01')
      )
    ORDER BY
      L_SHIPMODE ASC
  ) AS F0_0
