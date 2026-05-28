SELECT
  F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
  F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
  F0_0.P_BRAND AS P_BRAND,
  F0_0.P_TYPE AS P_TYPE,
  F0_0.P_SIZE AS P_SIZE
FROM
  (
    SELECT
      F0_0.P_BRAND AS P_BRAND,
      F0_0.P_TYPE AS P_TYPE,
      F0_0.P_SIZE AS P_SIZE,
      COUNT(
        DISTINCT (
          CASE
            WHEN (1 = F0_0._SETPROV_DUP_COUNT) THEN F0_0.PS_SUPPKEY
            ELSE NULL
          END
        )
      ) OVER (
        PARTITION BY
          F0_0.P_BRAND,
          F0_0.P_TYPE,
          F0_0.P_SIZE
      ) AS SUPPLIER_CNT,
      F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY
    FROM
      (
        SELECT
          F0_0.PS_PARTKEY AS PS_PARTKEY,
          F0_0.PS_SUPPKEY AS PS_SUPPKEY,
          F0_0.P_PARTKEY AS P_PARTKEY,
          F0_0.P_BRAND AS P_BRAND,
          F0_0.P_TYPE AS P_TYPE,
          F0_0.P_SIZE AS P_SIZE,
          F1_0."NESTING_EVAL_1" AS "NESTING_EVAL_1",
          F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
          F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
          GREATEST(
            F0_0.left__SETPROV_DUP_COUNT,
            F1_0.right__SETPROV_DUP_COUNT
          ) AS _SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              F0_0.PS_PARTKEY AS PS_PARTKEY,
              F0_0.PS_SUPPKEY AS PS_SUPPKEY,
              F1_0.P_PARTKEY AS P_PARTKEY,
              F1_0.P_BRAND AS P_BRAND,
              F1_0.P_TYPE AS P_TYPE,
              F1_0.P_SIZE AS P_SIZE,
              F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
              F1_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
              GREATEST(F0_0._SETPROV_DUP_COUNT, F1_0._SETPROV_DUP_COUNT) AS left__SETPROV_DUP_COUNT
            FROM
              (
                (
                  SELECT
                    F0_0.PS_PARTKEY AS PS_PARTKEY,
                    F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                    F0_0.PS_PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                    1 AS _SETPROV_DUP_COUNT
                  FROM
                    PARTSUPP F0_0
                ) F0_0
                CROSS JOIN (
                  SELECT
                    F0_0.P_PARTKEY AS P_PARTKEY,
                    F0_0.P_BRAND AS P_BRAND,
                    F0_0.P_TYPE AS P_TYPE,
                    F0_0.P_SIZE AS P_SIZE,
                    F0_0.P_PARTKEY AS PROV_PART_P__PARTKEY,
                    1 AS _SETPROV_DUP_COUNT
                  FROM
                    PART F0_0
                ) F1_0
              )
          ) F0_0,
          LATERAL (
            SELECT
              (
                CASE
                  WHEN ((F0_1."NESTING_EVAL_1") IS NULL) THEN TRUE
                  WHEN (F0_1."NESTING_EVAL_1" = 1) THEN NULL
                  ELSE (F0_1."NESTING_EVAL_1" = 2)
                END
              ) AS "NESTING_EVAL_1",
              ROW_NUMBER() OVER () AS right__SETPROV_DUP_COUNT
            FROM
              (
                SELECT
                  F0_1.NESTING_EVAL_HELP AS NESTING_EVAL_HELP,
                  F0_1._RESULT_TID AS _RESULT_TID,
                  MIN(F0_1.NESTING_EVAL_HELP) OVER () AS "NESTING_EVAL_1",
                  COUNT(1) OVER () AS __DUMMY_CNT
                FROM
                  (
                    (
                      SELECT
                        (
                          CASE
                            WHEN (F0_0.PS_SUPPKEY <> F0_1.S_SUPPKEY) THEN 2
                            WHEN (
                              ((F0_0.PS_SUPPKEY) IS NULL)
                              OR ((F0_1.S_SUPPKEY) IS NULL)
                            ) THEN 1
                            ELSE 0
                          END
                        ) AS NESTING_EVAL_HELP,
                        F0_1._RESULT_TID AS _RESULT_TID
                      FROM
                        (
                          SELECT
                            F0_1.S_SUPPKEY AS S_SUPPKEY,
                            F0_1.S_COMMENT AS S_COMMENT,
                            F0_1.rowid AS _RESULT_TID
                          FROM
                            SUPPLIER F0_1
                        ) F0_1
                      WHERE
                        (F0_1.S_COMMENT LIKE '%Customer%Complaints%')
                      UNION ALL
                      (
                        SELECT
                          NULL AS NESTING_EVAL_HELP,
                          -1 AS _RESULT_TID
                      )
                    )
                  ) F0_1
              ) F0_1
            WHERE
              (
                (F0_1.__DUMMY_CNT = 1)
                OR (F0_1._RESULT_TID <> -1)
              )
          ) F1_0
      ) F0_0
    WHERE
      (
        (
          (F0_0.P_PARTKEY = F0_0.PS_PARTKEY)
          AND (F0_0.P_BRAND <> 'Brand#45')
        )
        AND (
          NOT (
            (
              (
                (F0_0.P_TYPE LIKE 'MEDIUM POLISHED%')
                AND F0_0.P_SIZE IN (49, 14, 23, 45, 19, 3, 36, 9)
              )
              AND F0_0."NESTING_EVAL_1"
            )
          )
        )
      )
    ORDER BY
      SUPPLIER_CNT DESC NULLS LAST,
      P_BRAND ASC NULLS LAST,
      P_TYPE ASC NULLS LAST,
      P_SIZE ASC NULLS LAST
  ) F0_0;
