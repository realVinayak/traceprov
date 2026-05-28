SELECT
  F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
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
          F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
          F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
          GREATEST(F0_0._SETPROV_DUP_COUNT, F1_0._SETPROV_DUP_COUNT) AS _SETPROV_DUP_COUNT
        FROM
          (
            (
              SELECT
                F0_0.O_ORDERKEY AS O_ORDERKEY,
                F0_0.O_ORDERDATE AS O_ORDERDATE,
                F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
                F0_0.O_ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                1 AS _SETPROV_DUP_COUNT
              FROM
                ORDERS F0_0
            ) F0_0
            JOIN (
              SELECT
                F0_0.L_ORDERKEY AS L_ORDERKEY,
                F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                ROW_NUMBER() OVER (
                  PARTITION BY
                    F0_0.L_ORDERKEY
                  ORDER BY
                    F0_0.L_ORDERKEY
                ) AS _SETPROV_DUP_COUNT
              FROM
                (
                  SELECT
                    F0_0.L_ORDERKEY AS L_ORDERKEY,
                    F0_0.L_COMMITDATE AS L_COMMITDATE,
                    F0_0.L_RECEIPTDATE AS L_RECEIPTDATE,
                    F0_0.L_ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
                  FROM
                    LINEITEM F0_0
                ) F0_0
              WHERE
                (F0_0.L_COMMITDATE < F0_0.L_RECEIPTDATE)
            ) F1_0 ON ((F0_0.O_ORDERKEY = F1_0.L_ORDERKEY))
          )
      ) F0_0
    WHERE
      (
        (F0_0.O_ORDERDATE >= '1993-07-01')
        AND (F0_0.O_ORDERDATE < '1993-10-01')
      )
    ORDER BY
      O_ORDERPRIORITY ASC NULLS LAST
  ) F0_0;
