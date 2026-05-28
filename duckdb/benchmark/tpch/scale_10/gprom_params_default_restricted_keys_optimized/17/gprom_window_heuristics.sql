SELECT
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
  F0_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY"
FROM
  (
    SELECT
      F0_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
      F0_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY",
      F0_0._RESULT_TID AS _RESULT_TID,
      F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT,
      COUNT(1) OVER () AS __DUMMY_CNT
    FROM
      (
        (
          SELECT
            F0_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
            F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
            F0_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY",
            F0_0._RESULT_TID AS _RESULT_TID,
            F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
          FROM
            (
              SELECT
                F0_0.L_PARTKEY AS L_PARTKEY,
                F0_0.L_QUANTITY AS L_QUANTITY,
                F0_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                F0_0.P_PARTKEY AS P_PARTKEY,
                F0_0.P_BRAND AS P_BRAND,
                F0_0.P_CONTAINER AS P_CONTAINER,
                F1_0."(0200000*AVG(L_QUANTITY))" AS "(0200000*AVG(L_QUANTITY))",
                F1_0."L_PARTKEY_1" AS "L_PARTKEY_1",
                F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                F1_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY",
                hash(F0_0._RESULT_TID, F1_0._RESULT_TID) AS _RESULT_TID,
                GREATEST(F0_0._SETPROV_DUP_COUNT, F1_0._SETPROV_DUP_COUNT) AS _SETPROV_DUP_COUNT
              FROM
                (
                  (
                    SELECT
                      F0_0.L_PARTKEY AS L_PARTKEY,
                      F0_0.L_QUANTITY AS L_QUANTITY,
                      F0_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                      F1_0.P_PARTKEY AS P_PARTKEY,
                      F1_0.P_BRAND AS P_BRAND,
                      F1_0.P_CONTAINER AS P_CONTAINER,
                      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                      F1_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                      hash(F0_0._RESULT_TID, F1_0._RESULT_TID) AS _RESULT_TID,
                      GREATEST(F0_0._SETPROV_DUP_COUNT, F1_0._SETPROV_DUP_COUNT) AS _SETPROV_DUP_COUNT
                    FROM
                      (
                        (
                          SELECT
                            F0_0.L_PARTKEY AS L_PARTKEY,
                            F0_0.L_QUANTITY AS L_QUANTITY,
                            F0_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                            F0_0.L_ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                            F0_0.rowid AS _RESULT_TID,
                            1 AS _SETPROV_DUP_COUNT
                          FROM
                            LINEITEM F0_0
                        ) F0_0
                        CROSS JOIN (
                          SELECT
                            F0_0.P_PARTKEY AS P_PARTKEY,
                            F0_0.P_BRAND AS P_BRAND,
                            F0_0.P_CONTAINER AS P_CONTAINER,
                            F0_0.P_PARTKEY AS PROV_PART_P__PARTKEY,
                            F0_0.rowid AS _RESULT_TID,
                            1 AS _SETPROV_DUP_COUNT
                          FROM
                            PART F0_0
                        ) F1_0
                      )
                  ) F0_0
                  CROSS JOIN (
                    SELECT
                      (0.200000 * F0_0."AGGR_0") AS "(0200000*AVG(L_QUANTITY))",
                      F0_0.L_PARTKEY AS "L_PARTKEY_1",
                      F0_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY",
                      DENSE_RANK () OVER (
                        ORDER BY
                          F0_0.L_PARTKEY
                      ) AS _RESULT_TID,
                      ROW_NUMBER() OVER (
                        PARTITION BY
                          F0_0.L_PARTKEY
                        ORDER BY
                          F0_0.L_PARTKEY
                      ) AS _SETPROV_DUP_COUNT
                    FROM
                      (
                        SELECT
                          F0_0.L_PARTKEY AS L_PARTKEY,
                          F0_0.L_QUANTITY AS L_QUANTITY,
                          F0_0.L_ORDERKEY AS "PROV_LINEITEM_1_L__ORDERKEY",
                          AVG(F0_0.L_QUANTITY) OVER (
                            PARTITION BY
                              F0_0.L_PARTKEY
                          ) AS "AGGR_0"
                        FROM
                          LINEITEM F0_0
                      ) F0_0
                  ) F1_0
                )
            ) F0_0
          WHERE
            (
              (
                (
                  (
                    (F0_0.P_PARTKEY = F0_0.L_PARTKEY)
                    AND (F0_0.P_BRAND = 'Brand#23')
                  )
                  AND (F0_0.P_CONTAINER = 'MED BOX')
                )
                AND (
                  F0_0.L_QUANTITY < F0_0."(0200000*AVG(L_QUANTITY))"
                )
              )
              AND (F0_0.P_PARTKEY = F0_0."L_PARTKEY_1")
            )
          UNION ALL
          (
            SELECT
              NULL AS L_EXTENDEDPRICE,
              NULL AS PROV_LINEITEM_L__ORDERKEY,
              NULL AS PROV_PART_P__PARTKEY,
              NULL AS "PROV_LINEITEM_1_L__ORDERKEY",
              -1 AS _RESULT_TID,
              NULL AS _SETPROV_DUP_COUNT
          )
        )
      ) F0_0
  ) F0_0
WHERE
  (
    (F0_0.__DUMMY_CNT = 1)
    OR (F0_0._RESULT_TID <> -1)
  );
