SELECT
  F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
  F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
  F0_0.C_COUNT AS C_COUNT
FROM
  (
    SELECT
      F0_0."AGGR_0" AS C_COUNT,
      COUNT(
        (
          CASE
            WHEN (1 = F0_0._SETPROV_DUP_COUNT) THEN 1
            ELSE NULL
          END
        )
      ) OVER (
        PARTITION BY
          F0_0."AGGR_0"
      ) AS CUSTDIST,
      F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY
    FROM
      (
        SELECT
          F0_0.C_CUSTKEY AS C_CUSTKEY,
          F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
          F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
          F0_0."AGGR_0" AS "AGGR_0",
          ROW_NUMBER() OVER (
            PARTITION BY
              F0_0.C_CUSTKEY
            ORDER BY
              F0_0.C_CUSTKEY
          ) AS _SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              F0_0.C_CUSTKEY AS C_CUSTKEY,
              F1_0.O_ORDERKEY AS O_ORDERKEY,
              F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
              F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
              COUNT(F1_0.O_ORDERKEY) OVER (
                PARTITION BY
                  F0_0.C_CUSTKEY
              ) AS "AGGR_0"
            FROM
              (
                (
                  SELECT
                    F0_0.C_CUSTKEY AS C_CUSTKEY,
                    F0_0.C_CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY
                  FROM
                    CUSTOMER F0_0
                ) F0_0
                LEFT OUTER JOIN (
                  SELECT
                    F0_0.O_ORDERKEY AS O_ORDERKEY,
                    F0_0.O_CUSTKEY AS O_CUSTKEY,
                    F0_0.O_COMMENT AS O_COMMENT,
                    F0_0.O_ORDERKEY AS PROV_ORDERS_O__ORDERKEY
                  FROM
                    ORDERS F0_0
                ) F1_0 ON (
                  (
                    (F0_0.C_CUSTKEY = F1_0.O_CUSTKEY)
                    AND (NOT ((F1_0.O_COMMENT LIKE '%special%requests%')))
                  )
                )
              )
          ) F0_0
      ) F0_0
    ORDER BY
      CUSTDIST DESC NULLS LAST,
      C_COUNT DESC NULLS LAST
  ) F0_0;
