SELECT
  F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
  F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
  F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
  F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
  F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
  F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
  F0_0.PS_PARTKEY AS PS_PARTKEY
FROM
  (
    SELECT
      F0_0.PS_PARTKEY AS PS_PARTKEY,
      F0_0.VALUE AS VALUE,
      F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
      F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
      F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
      F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY"
    FROM
      (
        SELECT
          F0_0.PS_PARTKEY AS PS_PARTKEY,
          F0_0.VALUE AS VALUE,
          F1_0."(SUM((PS_SUPPLYCOST*PS_AVAILQTY))*0000100)" AS "(SUM((PS_SUPPLYCOST*PS_AVAILQTY))*0000100)",
          F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
          F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
          F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
          F1_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
          F1_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
          F1_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY"
        FROM
          (
            (
              SELECT
                F0_0."GROUP_0" AS PS_PARTKEY,
                F0_0."AGGR_0" AS VALUE,
                F1_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                F1_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY
              FROM
                (
                  (
                    SELECT
                      SUM((F0_0.PS_SUPPLYCOST * F0_0.PS_AVAILQTY)) AS "AGGR_0",
                      F0_0.PS_PARTKEY AS "GROUP_0"
                    FROM
                      (
                        (
                          (
                            SELECT
                              F0_0.PS_PARTKEY AS PS_PARTKEY,
                              F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                              F0_0.PS_AVAILQTY AS PS_AVAILQTY,
                              F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST
                            FROM
                              PARTSUPP F0_0
                          ) F0_0
                          CROSS JOIN (
                            SELECT
                              F0_0.S_SUPPKEY AS S_SUPPKEY,
                              F0_0.S_NATIONKEY AS S_NATIONKEY
                            FROM
                              SUPPLIER F0_0
                          ) F1_0
                        )
                        CROSS JOIN (
                          SELECT
                            F0_0.N_NATIONKEY AS N_NATIONKEY,
                            F0_0.N_NAME AS N_NAME
                          FROM
                            NATION F0_0
                        ) F2_0
                      )
                    WHERE
                      (
                        (
                          (F0_0.PS_SUPPKEY = F1_0.S_SUPPKEY)
                          AND (F1_0.S_NATIONKEY = F2_0.N_NATIONKEY)
                        )
                        AND (F2_0.N_NAME = 'GERMANY')
                      )
                    GROUP BY
                      F0_0.PS_PARTKEY
                  ) F0_0
                  JOIN (
                    SELECT
                      F0_0.PS_PARTKEY AS "_P_SIDE_GROUP_0",
                      F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                      F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY
                    FROM
                      (
                        SELECT
                          F0_0.PS_PARTKEY AS PS_PARTKEY,
                          F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                          F0_0.S_SUPPKEY AS S_SUPPKEY,
                          F0_0.S_NATIONKEY AS S_NATIONKEY,
                          F1_0.N_NATIONKEY AS N_NATIONKEY,
                          F1_0.N_NAME AS N_NAME,
                          F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                          F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                          F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY
                        FROM
                          (
                            (
                              SELECT
                                F0_0.PS_PARTKEY AS PS_PARTKEY,
                                F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                                F1_0.S_SUPPKEY AS S_SUPPKEY,
                                F1_0.S_NATIONKEY AS S_NATIONKEY,
                                F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                                F1_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY
                              FROM
                                (
                                  (
                                    SELECT
                                      F0_0.PS_PARTKEY AS PS_PARTKEY,
                                      F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                                      F0_0.PS_PARTKEY AS PROV_PARTSUPP_PS__PARTKEY
                                    FROM
                                      PARTSUPP F0_0
                                  ) F0_0
                                  CROSS JOIN (
                                    SELECT
                                      F0_0.S_SUPPKEY AS S_SUPPKEY,
                                      F0_0.S_NATIONKEY AS S_NATIONKEY,
                                      F0_0.S_SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY
                                    FROM
                                      SUPPLIER F0_0
                                  ) F1_0
                                )
                            ) F0_0
                            CROSS JOIN (
                              SELECT
                                F0_0.N_NATIONKEY AS N_NATIONKEY,
                                F0_0.N_NAME AS N_NAME,
                                F0_0.N_NATIONKEY AS PROV_NATION_N__NATIONKEY
                              FROM
                                NATION F0_0
                            ) F1_0
                          )
                      ) F0_0
                    WHERE
                      (
                        (
                          (F0_0.PS_SUPPKEY = F0_0.S_SUPPKEY)
                          AND (F0_0.S_NATIONKEY = F0_0.N_NATIONKEY)
                        )
                        AND (F0_0.N_NAME = 'GERMANY')
                      )
                  ) F1_0 ON (
                    (
                      (F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")
                      OR (
                        (F0_0."GROUP_0" IS NULL)
                        AND (F1_0."_P_SIDE_GROUP_0" IS NULL)
                      )
                    )
                  )
                )
            ) F0_0
            CROSS JOIN (
              SELECT
                (F0_0."AGGR_0" * 0.000100) AS "(SUM((PS_SUPPLYCOST*PS_AVAILQTY))*0000100)",
                F1_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
                F1_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
                F1_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY"
              FROM
                (
                  (
                    SELECT
                      SUM((F0_0.PS_SUPPLYCOST * F0_0.PS_AVAILQTY)) AS "AGGR_0"
                    FROM
                      (
                        (
                          (
                            SELECT
                              F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                              F0_0.PS_AVAILQTY AS PS_AVAILQTY,
                              F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST
                            FROM
                              PARTSUPP F0_0
                          ) F0_0
                          CROSS JOIN (
                            SELECT
                              F0_0.S_SUPPKEY AS S_SUPPKEY,
                              F0_0.S_NATIONKEY AS S_NATIONKEY
                            FROM
                              SUPPLIER F0_0
                          ) F1_0
                        )
                        CROSS JOIN (
                          SELECT
                            F0_0.N_NATIONKEY AS N_NATIONKEY,
                            F0_0.N_NAME AS N_NAME
                          FROM
                            NATION F0_0
                        ) F2_0
                      )
                    WHERE
                      (
                        (
                          (F0_0.PS_SUPPKEY = F1_0.S_SUPPKEY)
                          AND (F1_0.S_NATIONKEY = F2_0.N_NATIONKEY)
                        )
                        AND (F2_0.N_NAME = 'GERMANY')
                      )
                  ) F0_0
                  LEFT OUTER JOIN (
                    SELECT
                      F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
                      F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
                      F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY"
                    FROM
                      (
                        SELECT
                          F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                          F0_0.S_SUPPKEY AS S_SUPPKEY,
                          F0_0.S_NATIONKEY AS S_NATIONKEY,
                          F1_0.N_NATIONKEY AS N_NATIONKEY,
                          F1_0.N_NAME AS N_NAME,
                          F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
                          F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
                          F1_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY"
                        FROM
                          (
                            (
                              SELECT
                                F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                                F1_0.S_SUPPKEY AS S_SUPPKEY,
                                F1_0.S_NATIONKEY AS S_NATIONKEY,
                                F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
                                F1_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY"
                              FROM
                                (
                                  (
                                    SELECT
                                      F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                                      F0_0.PS_PARTKEY AS "PROV_PARTSUPP_1_PS__PARTKEY"
                                    FROM
                                      PARTSUPP F0_0
                                  ) F0_0
                                  CROSS JOIN (
                                    SELECT
                                      F0_0.S_SUPPKEY AS S_SUPPKEY,
                                      F0_0.S_NATIONKEY AS S_NATIONKEY,
                                      F0_0.S_SUPPKEY AS "PROV_SUPPLIER_1_S__SUPPKEY"
                                    FROM
                                      SUPPLIER F0_0
                                  ) F1_0
                                )
                            ) F0_0
                            CROSS JOIN (
                              SELECT
                                F0_0.N_NATIONKEY AS N_NATIONKEY,
                                F0_0.N_NAME AS N_NAME,
                                F0_0.N_NATIONKEY AS "PROV_NATION_1_N__NATIONKEY"
                              FROM
                                NATION F0_0
                            ) F1_0
                          )
                      ) F0_0
                    WHERE
                      (
                        (
                          (F0_0.PS_SUPPKEY = F0_0.S_SUPPKEY)
                          AND (F0_0.S_NATIONKEY = F0_0.N_NATIONKEY)
                        )
                        AND (F0_0.N_NAME = 'GERMANY')
                      )
                  ) F1_0 ON ((1 = 1))
                )
            ) F1_0
          )
      ) F0_0
    WHERE
      (
        F0_0.VALUE > F0_0."(SUM((PS_SUPPLYCOST*PS_AVAILQTY))*0000100)"
      )
    ORDER BY
      VALUE DESC NULLS LAST
  ) F0_0;
