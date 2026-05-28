WITH
  temp_view_1 AS (
    SELECT
      /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0",
      F0_0."PS_PARTKEY_1" AS "PS_PARTKEY_1",
      F1_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
      F1_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
      F1_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
      F1_0."PROV_REGION_1_R__REGIONKEY" AS "PROV_REGION_1_R__REGIONKEY"
    FROM
      (
        (
          SELECT
            MIN(F1_0.PS_SUPPLYCOST) AS "AGGR_0",
            F1_0.PS_PARTKEY AS "PS_PARTKEY_1"
          FROM
            (
              (
                (
                  SUPPLIER F0_0
                  CROSS JOIN PARTSUPP F1_0
                )
                CROSS JOIN NATION F2_0
              )
              CROSS JOIN REGION F3_0
            )
          WHERE
            (
              (
                (
                  (F0_0.S_SUPPKEY = F1_0.PS_SUPPKEY)
                  AND (F0_0.S_NATIONKEY = F2_0.N_NATIONKEY)
                )
                AND (F2_0.N_REGIONKEY = F3_0.R_REGIONKEY)
              )
              AND (F3_0.R_NAME = 'EUROPE')
            )
          GROUP BY
            F1_0.PS_PARTKEY
        ) F0_0
        JOIN (
          SELECT
            F0_0.PS_PARTKEY AS "_P_SIDE_PS_PARTKEY_1",
            F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
            F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
            F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
            F0_0."PROV_REGION_1_R__REGIONKEY" AS "PROV_REGION_1_R__REGIONKEY"
          FROM
            (
              SELECT
                F0_0.S_SUPPKEY AS S_SUPPKEY,
                F0_0.S_NAME AS S_NAME,
                F0_0.S_ADDRESS AS S_ADDRESS,
                F0_0.S_NATIONKEY AS S_NATIONKEY,
                F0_0.S_PHONE AS S_PHONE,
                F0_0.S_ACCTBAL AS S_ACCTBAL,
                F0_0.S_COMMENT AS S_COMMENT,
                F0_0.PS_PARTKEY AS PS_PARTKEY,
                F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                F0_0.PS_AVAILQTY AS PS_AVAILQTY,
                F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
                F0_0.PS_COMMENT AS PS_COMMENT,
                F0_0.N_NATIONKEY AS N_NATIONKEY,
                F0_0.N_NAME AS N_NAME,
                F0_0.N_REGIONKEY AS N_REGIONKEY,
                F0_0.N_COMMENT AS N_COMMENT,
                F1_0.R_REGIONKEY AS R_REGIONKEY,
                F1_0.R_NAME AS R_NAME,
                F1_0.R_COMMENT AS R_COMMENT,
                F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
                F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
                F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
                F1_0."PROV_REGION_1_R__REGIONKEY" AS "PROV_REGION_1_R__REGIONKEY"
              FROM
                (
                  (
                    SELECT
                      F0_0.S_SUPPKEY AS S_SUPPKEY,
                      F0_0.S_NAME AS S_NAME,
                      F0_0.S_ADDRESS AS S_ADDRESS,
                      F0_0.S_NATIONKEY AS S_NATIONKEY,
                      F0_0.S_PHONE AS S_PHONE,
                      F0_0.S_ACCTBAL AS S_ACCTBAL,
                      F0_0.S_COMMENT AS S_COMMENT,
                      F0_0.PS_PARTKEY AS PS_PARTKEY,
                      F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                      F0_0.PS_AVAILQTY AS PS_AVAILQTY,
                      F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
                      F0_0.PS_COMMENT AS PS_COMMENT,
                      F1_0.N_NATIONKEY AS N_NATIONKEY,
                      F1_0.N_NAME AS N_NAME,
                      F1_0.N_REGIONKEY AS N_REGIONKEY,
                      F1_0.N_COMMENT AS N_COMMENT,
                      F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
                      F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
                      F1_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY"
                    FROM
                      (
                        (
                          SELECT
                            F0_0.S_SUPPKEY AS S_SUPPKEY,
                            F0_0.S_NAME AS S_NAME,
                            F0_0.S_ADDRESS AS S_ADDRESS,
                            F0_0.S_NATIONKEY AS S_NATIONKEY,
                            F0_0.S_PHONE AS S_PHONE,
                            F0_0.S_ACCTBAL AS S_ACCTBAL,
                            F0_0.S_COMMENT AS S_COMMENT,
                            F1_0.PS_PARTKEY AS PS_PARTKEY,
                            F1_0.PS_SUPPKEY AS PS_SUPPKEY,
                            F1_0.PS_AVAILQTY AS PS_AVAILQTY,
                            F1_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
                            F1_0.PS_COMMENT AS PS_COMMENT,
                            F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
                            F1_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY"
                          FROM
                            (
                              (
                                SELECT
                                  F0_0.S_SUPPKEY AS S_SUPPKEY,
                                  F0_0.S_NAME AS S_NAME,
                                  F0_0.S_ADDRESS AS S_ADDRESS,
                                  F0_0.S_NATIONKEY AS S_NATIONKEY,
                                  F0_0.S_PHONE AS S_PHONE,
                                  F0_0.S_ACCTBAL AS S_ACCTBAL,
                                  F0_0.S_COMMENT AS S_COMMENT,
                                  F0_0.S_SUPPKEY AS "PROV_SUPPLIER_1_S__SUPPKEY"
                                FROM
                                  SUPPLIER F0_0
                              ) F0_0
                              CROSS JOIN (
                                SELECT
                                  F0_0.PS_PARTKEY AS PS_PARTKEY,
                                  F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                                  F0_0.PS_AVAILQTY AS PS_AVAILQTY,
                                  F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
                                  F0_0.PS_COMMENT AS PS_COMMENT,
                                  F0_0.PS_PARTKEY AS "PROV_PARTSUPP_1_PS__PARTKEY"
                                FROM
                                  PARTSUPP F0_0
                              ) F1_0
                            )
                        ) F0_0
                        CROSS JOIN (
                          SELECT
                            F0_0.N_NATIONKEY AS N_NATIONKEY,
                            F0_0.N_NAME AS N_NAME,
                            F0_0.N_REGIONKEY AS N_REGIONKEY,
                            F0_0.N_COMMENT AS N_COMMENT,
                            F0_0.N_NATIONKEY AS "PROV_NATION_1_N__NATIONKEY"
                          FROM
                            NATION F0_0
                        ) F1_0
                      )
                  ) F0_0
                  CROSS JOIN (
                    SELECT
                      F0_0.R_REGIONKEY AS R_REGIONKEY,
                      F0_0.R_NAME AS R_NAME,
                      F0_0.R_COMMENT AS R_COMMENT,
                      F0_0.R_REGIONKEY AS "PROV_REGION_1_R__REGIONKEY"
                    FROM
                      REGION F0_0
                  ) F1_0
                )
            ) F0_0
          WHERE
            (
              (
                (
                  (F0_0.S_SUPPKEY = F0_0.PS_SUPPKEY)
                  AND (F0_0.S_NATIONKEY = F0_0.N_NATIONKEY)
                )
                AND (F0_0.N_REGIONKEY = F0_0.R_REGIONKEY)
              )
              AND (F0_0.R_NAME = 'EUROPE')
            )
        ) F1_0 ON (
          (
            (F0_0."PS_PARTKEY_1" = F1_0."_P_SIDE_PS_PARTKEY_1")
            OR (
              (F0_0."PS_PARTKEY_1" IS NULL)
              AND (F1_0."_P_SIDE_PS_PARTKEY_1" IS NULL)
            )
          )
        )
      )
  ),
  temp_view_0 AS (
    SELECT
      /*+ materialize */ F0_0.S_ACCTBAL AS S_ACCTBAL,
      F0_0.S_NAME AS S_NAME,
      F0_0.N_NAME AS N_NAME,
      F0_0.P_PARTKEY AS P_PARTKEY,
      F0_0.P_MFGR AS P_MFGR,
      F0_0.S_ADDRESS AS S_ADDRESS,
      F0_0.S_PHONE AS S_PHONE,
      F0_0.S_COMMENT AS S_COMMENT,
      F1_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
      F1_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F1_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
      F1_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY,
      F1_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
      F1_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
      F1_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
      F1_0."PROV_REGION_1_R__REGIONKEY" AS "PROV_REGION_1_R__REGIONKEY"
    FROM
      (
        (
          SELECT
            F1_0.S_ACCTBAL AS S_ACCTBAL,
            F1_0.S_NAME AS S_NAME,
            F3_0.N_NAME AS N_NAME,
            F0_0.P_PARTKEY AS P_PARTKEY,
            F0_0.P_MFGR AS P_MFGR,
            F1_0.S_ADDRESS AS S_ADDRESS,
            F1_0.S_PHONE AS S_PHONE,
            F1_0.S_COMMENT AS S_COMMENT
          FROM
            (
              (
                (
                  (
                    (
                      PART F0_0
                      CROSS JOIN SUPPLIER F1_0
                    )
                    CROSS JOIN PARTSUPP F2_0
                  )
                  CROSS JOIN NATION F3_0
                )
                CROSS JOIN REGION F4_0
              )
              CROSS JOIN (
                SELECT
                  MIN(F1_0.PS_SUPPLYCOST) AS "MIN(PS_SUPPLYCOST)",
                  F1_0.PS_PARTKEY AS "PS_PARTKEY_1"
                FROM
                  (
                    (
                      (
                        SUPPLIER F0_0
                        CROSS JOIN PARTSUPP F1_0
                      )
                      CROSS JOIN NATION F2_0
                    )
                    CROSS JOIN REGION F3_0
                  )
                WHERE
                  (
                    (
                      (
                        (F0_0.S_SUPPKEY = F1_0.PS_SUPPKEY)
                        AND (F0_0.S_NATIONKEY = F2_0.N_NATIONKEY)
                      )
                      AND (F2_0.N_REGIONKEY = F3_0.R_REGIONKEY)
                    )
                    AND (F3_0.R_NAME = 'EUROPE')
                  )
                GROUP BY
                  F1_0.PS_PARTKEY
              ) F5_0
            )
          WHERE
            (
              (
                (
                  (
                    (
                      (
                        (
                          (
                            (F0_0.P_PARTKEY = F2_0.PS_PARTKEY)
                            AND (F1_0.S_SUPPKEY = F2_0.PS_SUPPKEY)
                          )
                          AND (F0_0.P_SIZE = 15)
                        )
                        AND (F0_0.P_TYPE LIKE '%BRASS')
                      )
                      AND (F1_0.S_NATIONKEY = F3_0.N_NATIONKEY)
                    )
                    AND (F3_0.N_REGIONKEY = F4_0.R_REGIONKEY)
                  )
                  AND (F4_0.R_NAME = 'EUROPE')
                )
                AND (F2_0.PS_SUPPLYCOST = F5_0."MIN(PS_SUPPLYCOST)")
              )
              AND (F0_0.P_PARTKEY = F5_0."PS_PARTKEY_1")
            )
          ORDER BY
            S_ACCTBAL DESC NULLS LAST,
            N_NAME ASC NULLS LAST,
            S_NAME ASC NULLS LAST,
            P_PARTKEY ASC NULLS LAST
          LIMIT
            100
        ) F0_0
        JOIN (
          SELECT
            F0_0.S_ACCTBAL AS S_ACCTBAL,
            F0_0.S_NAME AS S_NAME,
            F0_0.N_NAME AS N_NAME,
            F0_0.P_PARTKEY AS P_PARTKEY,
            F0_0.P_MFGR AS P_MFGR,
            F0_0.S_ADDRESS AS S_ADDRESS,
            F0_0.S_PHONE AS S_PHONE,
            F0_0.S_COMMENT AS S_COMMENT,
            F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
            F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
            F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
            F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
            F0_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY,
            F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
            F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
            F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
            F0_0."PROV_REGION_1_R__REGIONKEY" AS "PROV_REGION_1_R__REGIONKEY"
          FROM
            (
              SELECT
                F0_0.P_PARTKEY AS P_PARTKEY,
                F0_0.P_NAME AS P_NAME,
                F0_0.P_MFGR AS P_MFGR,
                F0_0.P_BRAND AS P_BRAND,
                F0_0.P_TYPE AS P_TYPE,
                F0_0.P_SIZE AS P_SIZE,
                F0_0.P_CONTAINER AS P_CONTAINER,
                F0_0.P_RETAILPRICE AS P_RETAILPRICE,
                F0_0.P_COMMENT AS P_COMMENT,
                F0_0.S_SUPPKEY AS S_SUPPKEY,
                F0_0.S_NAME AS S_NAME,
                F0_0.S_ADDRESS AS S_ADDRESS,
                F0_0.S_NATIONKEY AS S_NATIONKEY,
                F0_0.S_PHONE AS S_PHONE,
                F0_0.S_ACCTBAL AS S_ACCTBAL,
                F0_0.S_COMMENT AS S_COMMENT,
                F0_0.PS_PARTKEY AS PS_PARTKEY,
                F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                F0_0.PS_AVAILQTY AS PS_AVAILQTY,
                F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
                F0_0.PS_COMMENT AS PS_COMMENT,
                F0_0.N_NATIONKEY AS N_NATIONKEY,
                F0_0.N_NAME AS N_NAME,
                F0_0.N_REGIONKEY AS N_REGIONKEY,
                F0_0.N_COMMENT AS N_COMMENT,
                F0_0.R_REGIONKEY AS R_REGIONKEY,
                F0_0.R_NAME AS R_NAME,
                F0_0.R_COMMENT AS R_COMMENT,
                F1_0."MIN(PS_SUPPLYCOST)" AS "MIN(PS_SUPPLYCOST)",
                F1_0."PS_PARTKEY_1" AS "PS_PARTKEY_1",
                F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
                F0_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY,
                F1_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
                F1_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
                F1_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
                F1_0."PROV_REGION_1_R__REGIONKEY" AS "PROV_REGION_1_R__REGIONKEY"
              FROM
                (
                  (
                    SELECT
                      F0_0.P_PARTKEY AS P_PARTKEY,
                      F0_0.P_NAME AS P_NAME,
                      F0_0.P_MFGR AS P_MFGR,
                      F0_0.P_BRAND AS P_BRAND,
                      F0_0.P_TYPE AS P_TYPE,
                      F0_0.P_SIZE AS P_SIZE,
                      F0_0.P_CONTAINER AS P_CONTAINER,
                      F0_0.P_RETAILPRICE AS P_RETAILPRICE,
                      F0_0.P_COMMENT AS P_COMMENT,
                      F0_0.S_SUPPKEY AS S_SUPPKEY,
                      F0_0.S_NAME AS S_NAME,
                      F0_0.S_ADDRESS AS S_ADDRESS,
                      F0_0.S_NATIONKEY AS S_NATIONKEY,
                      F0_0.S_PHONE AS S_PHONE,
                      F0_0.S_ACCTBAL AS S_ACCTBAL,
                      F0_0.S_COMMENT AS S_COMMENT,
                      F0_0.PS_PARTKEY AS PS_PARTKEY,
                      F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                      F0_0.PS_AVAILQTY AS PS_AVAILQTY,
                      F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
                      F0_0.PS_COMMENT AS PS_COMMENT,
                      F0_0.N_NATIONKEY AS N_NATIONKEY,
                      F0_0.N_NAME AS N_NAME,
                      F0_0.N_REGIONKEY AS N_REGIONKEY,
                      F0_0.N_COMMENT AS N_COMMENT,
                      F1_0.R_REGIONKEY AS R_REGIONKEY,
                      F1_0.R_NAME AS R_NAME,
                      F1_0.R_COMMENT AS R_COMMENT,
                      F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                      F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                      F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
                      F1_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY
                    FROM
                      (
                        (
                          SELECT
                            F0_0.P_PARTKEY AS P_PARTKEY,
                            F0_0.P_NAME AS P_NAME,
                            F0_0.P_MFGR AS P_MFGR,
                            F0_0.P_BRAND AS P_BRAND,
                            F0_0.P_TYPE AS P_TYPE,
                            F0_0.P_SIZE AS P_SIZE,
                            F0_0.P_CONTAINER AS P_CONTAINER,
                            F0_0.P_RETAILPRICE AS P_RETAILPRICE,
                            F0_0.P_COMMENT AS P_COMMENT,
                            F0_0.S_SUPPKEY AS S_SUPPKEY,
                            F0_0.S_NAME AS S_NAME,
                            F0_0.S_ADDRESS AS S_ADDRESS,
                            F0_0.S_NATIONKEY AS S_NATIONKEY,
                            F0_0.S_PHONE AS S_PHONE,
                            F0_0.S_ACCTBAL AS S_ACCTBAL,
                            F0_0.S_COMMENT AS S_COMMENT,
                            F0_0.PS_PARTKEY AS PS_PARTKEY,
                            F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                            F0_0.PS_AVAILQTY AS PS_AVAILQTY,
                            F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
                            F0_0.PS_COMMENT AS PS_COMMENT,
                            F1_0.N_NATIONKEY AS N_NATIONKEY,
                            F1_0.N_NAME AS N_NAME,
                            F1_0.N_REGIONKEY AS N_REGIONKEY,
                            F1_0.N_COMMENT AS N_COMMENT,
                            F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                            F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                            F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                            F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY
                          FROM
                            (
                              (
                                SELECT
                                  F0_0.P_PARTKEY AS P_PARTKEY,
                                  F0_0.P_NAME AS P_NAME,
                                  F0_0.P_MFGR AS P_MFGR,
                                  F0_0.P_BRAND AS P_BRAND,
                                  F0_0.P_TYPE AS P_TYPE,
                                  F0_0.P_SIZE AS P_SIZE,
                                  F0_0.P_CONTAINER AS P_CONTAINER,
                                  F0_0.P_RETAILPRICE AS P_RETAILPRICE,
                                  F0_0.P_COMMENT AS P_COMMENT,
                                  F0_0.S_SUPPKEY AS S_SUPPKEY,
                                  F0_0.S_NAME AS S_NAME,
                                  F0_0.S_ADDRESS AS S_ADDRESS,
                                  F0_0.S_NATIONKEY AS S_NATIONKEY,
                                  F0_0.S_PHONE AS S_PHONE,
                                  F0_0.S_ACCTBAL AS S_ACCTBAL,
                                  F0_0.S_COMMENT AS S_COMMENT,
                                  F1_0.PS_PARTKEY AS PS_PARTKEY,
                                  F1_0.PS_SUPPKEY AS PS_SUPPKEY,
                                  F1_0.PS_AVAILQTY AS PS_AVAILQTY,
                                  F1_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
                                  F1_0.PS_COMMENT AS PS_COMMENT,
                                  F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                                  F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                                  F1_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY
                                FROM
                                  (
                                    (
                                      SELECT
                                        F0_0.P_PARTKEY AS P_PARTKEY,
                                        F0_0.P_NAME AS P_NAME,
                                        F0_0.P_MFGR AS P_MFGR,
                                        F0_0.P_BRAND AS P_BRAND,
                                        F0_0.P_TYPE AS P_TYPE,
                                        F0_0.P_SIZE AS P_SIZE,
                                        F0_0.P_CONTAINER AS P_CONTAINER,
                                        F0_0.P_RETAILPRICE AS P_RETAILPRICE,
                                        F0_0.P_COMMENT AS P_COMMENT,
                                        F1_0.S_SUPPKEY AS S_SUPPKEY,
                                        F1_0.S_NAME AS S_NAME,
                                        F1_0.S_ADDRESS AS S_ADDRESS,
                                        F1_0.S_NATIONKEY AS S_NATIONKEY,
                                        F1_0.S_PHONE AS S_PHONE,
                                        F1_0.S_ACCTBAL AS S_ACCTBAL,
                                        F1_0.S_COMMENT AS S_COMMENT,
                                        F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                                        F1_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY
                                      FROM
                                        (
                                          (
                                            SELECT
                                              F0_0.P_PARTKEY AS P_PARTKEY,
                                              F0_0.P_NAME AS P_NAME,
                                              F0_0.P_MFGR AS P_MFGR,
                                              F0_0.P_BRAND AS P_BRAND,
                                              F0_0.P_TYPE AS P_TYPE,
                                              F0_0.P_SIZE AS P_SIZE,
                                              F0_0.P_CONTAINER AS P_CONTAINER,
                                              F0_0.P_RETAILPRICE AS P_RETAILPRICE,
                                              F0_0.P_COMMENT AS P_COMMENT,
                                              F0_0.P_PARTKEY AS PROV_PART_P__PARTKEY
                                            FROM
                                              PART F0_0
                                          ) F0_0
                                          CROSS JOIN (
                                            SELECT
                                              F0_0.S_SUPPKEY AS S_SUPPKEY,
                                              F0_0.S_NAME AS S_NAME,
                                              F0_0.S_ADDRESS AS S_ADDRESS,
                                              F0_0.S_NATIONKEY AS S_NATIONKEY,
                                              F0_0.S_PHONE AS S_PHONE,
                                              F0_0.S_ACCTBAL AS S_ACCTBAL,
                                              F0_0.S_COMMENT AS S_COMMENT,
                                              F0_0.S_SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY
                                            FROM
                                              SUPPLIER F0_0
                                          ) F1_0
                                        )
                                    ) F0_0
                                    CROSS JOIN (
                                      SELECT
                                        F0_0.PS_PARTKEY AS PS_PARTKEY,
                                        F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                                        F0_0.PS_AVAILQTY AS PS_AVAILQTY,
                                        F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
                                        F0_0.PS_COMMENT AS PS_COMMENT,
                                        F0_0.PS_PARTKEY AS PROV_PARTSUPP_PS__PARTKEY
                                      FROM
                                        PARTSUPP F0_0
                                    ) F1_0
                                  )
                              ) F0_0
                              CROSS JOIN (
                                SELECT
                                  F0_0.N_NATIONKEY AS N_NATIONKEY,
                                  F0_0.N_NAME AS N_NAME,
                                  F0_0.N_REGIONKEY AS N_REGIONKEY,
                                  F0_0.N_COMMENT AS N_COMMENT,
                                  F0_0.N_NATIONKEY AS PROV_NATION_N__NATIONKEY
                                FROM
                                  NATION F0_0
                              ) F1_0
                            )
                        ) F0_0
                        CROSS JOIN (
                          SELECT
                            F0_0.R_REGIONKEY AS R_REGIONKEY,
                            F0_0.R_NAME AS R_NAME,
                            F0_0.R_COMMENT AS R_COMMENT,
                            F0_0.R_REGIONKEY AS PROV_REGION_R__REGIONKEY
                          FROM
                            REGION F0_0
                        ) F1_0
                      )
                  ) F0_0
                  CROSS JOIN (
                    SELECT
                      F0_0."AGGR_0" AS "MIN(PS_SUPPLYCOST)",
                      F0_0."PS_PARTKEY_1" AS "PS_PARTKEY_1",
                      F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
                      F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
                      F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
                      F0_0."PROV_REGION_1_R__REGIONKEY" AS "PROV_REGION_1_R__REGIONKEY"
                    FROM
                      (
                        SELECT
                          *
                        FROM
                          temp_view_1
                      ) F0_0
                  ) F1_0
                )
            ) F0_0
          WHERE
            (
              (
                (
                  (
                    (
                      (
                        (
                          (
                            (F0_0.P_PARTKEY = F0_0.PS_PARTKEY)
                            AND (F0_0.S_SUPPKEY = F0_0.PS_SUPPKEY)
                          )
                          AND (F0_0.P_SIZE = 15)
                        )
                        AND (F0_0.P_TYPE LIKE '%BRASS')
                      )
                      AND (F0_0.S_NATIONKEY = F0_0.N_NATIONKEY)
                    )
                    AND (F0_0.N_REGIONKEY = F0_0.R_REGIONKEY)
                  )
                  AND (F0_0.R_NAME = 'EUROPE')
                )
                AND (F0_0.PS_SUPPLYCOST = F0_0."MIN(PS_SUPPLYCOST)")
              )
              AND (F0_0.P_PARTKEY = F0_0."PS_PARTKEY_1")
            )
          ORDER BY
            S_ACCTBAL DESC NULLS LAST,
            N_NAME ASC NULLS LAST,
            S_NAME ASC NULLS LAST,
            P_PARTKEY ASC NULLS LAST
        ) F1_0 ON (
          (
            (
              (
                (
                  (
                    (
                      (
                        (F0_0.S_ACCTBAL = F1_0.S_ACCTBAL)
                        AND (F0_0.S_NAME = F1_0.S_NAME)
                      )
                      AND (F0_0.N_NAME = F1_0.N_NAME)
                    )
                    AND (F0_0.P_PARTKEY = F1_0.P_PARTKEY)
                  )
                  AND (F0_0.P_MFGR = F1_0.P_MFGR)
                )
                AND (F0_0.S_ADDRESS = F1_0.S_ADDRESS)
              )
              AND (F0_0.S_PHONE = F1_0.S_PHONE)
            )
            AND (F0_0.S_COMMENT = F1_0.S_COMMENT)
          )
        )
      )
  )
SELECT
  F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
  F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
  F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
  F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
  F0_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY,
  F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
  F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
  F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
  F0_0."PROV_REGION_1_R__REGIONKEY" AS "PROV_REGION_1_R__REGIONKEY",
  F0_0.S_ACCTBAL AS S_ACCTBAL,
  F0_0.S_NAME AS S_NAME,
  F0_0.N_NAME AS N_NAME,
  F0_0.P_PARTKEY AS P_PARTKEY,
  F0_0.P_MFGR AS P_MFGR,
  F0_0.S_ADDRESS AS S_ADDRESS,
  F0_0.S_PHONE AS S_PHONE,
  F0_0.S_COMMENT AS S_COMMENT
FROM
  (
    SELECT
      *
    FROM
      temp_view_0
  ) F0_0;
