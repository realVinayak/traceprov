SELECT
  F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
  F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
  F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
  F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY
FROM
  (
    SELECT
      F0_0."GROUP_0" AS NATION,
      F0_0."GROUP_1" AS O_YEAR,
      F1_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
      F1_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F1_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY
    FROM
      (
        (
          SELECT
            F5_0.N_NAME AS "GROUP_0",
            DATE_PART('YEAR', (F4_0.O_ORDERDATE)::DATE) AS "GROUP_1"
          FROM
            (
              (
                (
                  (
                    (
                      (
                        SELECT
                          F0_0.P_PARTKEY AS P_PARTKEY,
                          F0_0.P_NAME AS P_NAME
                        FROM
                          PART F0_0
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
                        F0_0.L_ORDERKEY AS L_ORDERKEY,
                        F0_0.L_PARTKEY AS L_PARTKEY,
                        F0_0.L_SUPPKEY AS L_SUPPKEY
                      FROM
                        LINEITEM F0_0
                    ) F2_0
                  )
                  CROSS JOIN (
                    SELECT
                      F0_0.PS_PARTKEY AS PS_PARTKEY,
                      F0_0.PS_SUPPKEY AS PS_SUPPKEY
                    FROM
                      PARTSUPP F0_0
                  ) F3_0
                )
                CROSS JOIN (
                  SELECT
                    F0_0.O_ORDERKEY AS O_ORDERKEY,
                    F0_0.O_ORDERDATE AS O_ORDERDATE
                  FROM
                    ORDERS F0_0
                ) F4_0
              )
              CROSS JOIN (
                SELECT
                  F0_0.N_NATIONKEY AS N_NATIONKEY,
                  F0_0.N_NAME AS N_NAME
                FROM
                  NATION F0_0
              ) F5_0
            )
          WHERE
            (
              (
                (
                  (
                    (
                      (
                        (F1_0.S_SUPPKEY = F2_0.L_SUPPKEY)
                        AND (F3_0.PS_SUPPKEY = F2_0.L_SUPPKEY)
                      )
                      AND (F3_0.PS_PARTKEY = F2_0.L_PARTKEY)
                    )
                    AND (F0_0.P_PARTKEY = F2_0.L_PARTKEY)
                  )
                  AND (F4_0.O_ORDERKEY = F2_0.L_ORDERKEY)
                )
                AND (F1_0.S_NATIONKEY = F5_0.N_NATIONKEY)
              )
              AND (F0_0.P_NAME LIKE '%green%')
            )
          GROUP BY
            F5_0.N_NAME,
            DATE_PART('YEAR', (F4_0.O_ORDERDATE)::DATE)
        ) F0_0
        JOIN (
          SELECT
            F0_0.N_NAME AS "_P_SIDE_GROUP_0",
            DATE_PART('YEAR', (F0_0.O_ORDERDATE)::DATE) AS "_P_SIDE_GROUP_1",
            F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
            F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
            F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
            F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
            F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY
          FROM
            (
              SELECT
                F0_0.P_PARTKEY AS P_PARTKEY,
                F0_0.P_NAME AS P_NAME,
                F0_0.S_SUPPKEY AS S_SUPPKEY,
                F0_0.S_NATIONKEY AS S_NATIONKEY,
                F0_0.L_ORDERKEY AS L_ORDERKEY,
                F0_0.L_PARTKEY AS L_PARTKEY,
                F0_0.L_SUPPKEY AS L_SUPPKEY,
                F0_0.PS_PARTKEY AS PS_PARTKEY,
                F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                F0_0.O_ORDERKEY AS O_ORDERKEY,
                F0_0.O_ORDERDATE AS O_ORDERDATE,
                F1_0.N_NATIONKEY AS N_NATIONKEY,
                F1_0.N_NAME AS N_NAME,
                F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY
              FROM
                (
                  (
                    SELECT
                      F0_0.P_PARTKEY AS P_PARTKEY,
                      F0_0.P_NAME AS P_NAME,
                      F0_0.S_SUPPKEY AS S_SUPPKEY,
                      F0_0.S_NATIONKEY AS S_NATIONKEY,
                      F0_0.L_ORDERKEY AS L_ORDERKEY,
                      F0_0.L_PARTKEY AS L_PARTKEY,
                      F0_0.L_SUPPKEY AS L_SUPPKEY,
                      F0_0.PS_PARTKEY AS PS_PARTKEY,
                      F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                      F1_0.O_ORDERKEY AS O_ORDERKEY,
                      F1_0.O_ORDERDATE AS O_ORDERDATE,
                      F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                      F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                      F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY
                    FROM
                      (
                        (
                          SELECT
                            F0_0.P_PARTKEY AS P_PARTKEY,
                            F0_0.P_NAME AS P_NAME,
                            F0_0.S_SUPPKEY AS S_SUPPKEY,
                            F0_0.S_NATIONKEY AS S_NATIONKEY,
                            F0_0.L_ORDERKEY AS L_ORDERKEY,
                            F0_0.L_PARTKEY AS L_PARTKEY,
                            F0_0.L_SUPPKEY AS L_SUPPKEY,
                            F1_0.PS_PARTKEY AS PS_PARTKEY,
                            F1_0.PS_SUPPKEY AS PS_SUPPKEY,
                            F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                            F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                            F1_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY
                          FROM
                            (
                              (
                                SELECT
                                  F0_0.P_PARTKEY AS P_PARTKEY,
                                  F0_0.P_NAME AS P_NAME,
                                  F0_0.S_SUPPKEY AS S_SUPPKEY,
                                  F0_0.S_NATIONKEY AS S_NATIONKEY,
                                  F1_0.L_ORDERKEY AS L_ORDERKEY,
                                  F1_0.L_PARTKEY AS L_PARTKEY,
                                  F1_0.L_SUPPKEY AS L_SUPPKEY,
                                  F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                                  F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                                  F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
                                FROM
                                  (
                                    (
                                      SELECT
                                        F0_0.P_PARTKEY AS P_PARTKEY,
                                        F0_0.P_NAME AS P_NAME,
                                        F1_0.S_SUPPKEY AS S_SUPPKEY,
                                        F1_0.S_NATIONKEY AS S_NATIONKEY,
                                        F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                                        F1_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY
                                      FROM
                                        (
                                          (
                                            SELECT
                                              F0_0.P_PARTKEY AS P_PARTKEY,
                                              F0_0.P_NAME AS P_NAME,
                                              F0_0.P_PARTKEY AS PROV_PART_P__PARTKEY
                                            FROM
                                              PART F0_0
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
                                        F0_0.L_ORDERKEY AS L_ORDERKEY,
                                        F0_0.L_PARTKEY AS L_PARTKEY,
                                        F0_0.L_SUPPKEY AS L_SUPPKEY,
                                        F0_0.L_ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
                                      FROM
                                        LINEITEM F0_0
                                    ) F1_0
                                  )
                              ) F0_0
                              CROSS JOIN (
                                SELECT
                                  F0_0.PS_PARTKEY AS PS_PARTKEY,
                                  F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                                  F0_0.PS_PARTKEY AS PROV_PARTSUPP_PS__PARTKEY
                                FROM
                                  PARTSUPP F0_0
                              ) F1_0
                            )
                        ) F0_0
                        CROSS JOIN (
                          SELECT
                            F0_0.O_ORDERKEY AS O_ORDERKEY,
                            F0_0.O_ORDERDATE AS O_ORDERDATE,
                            F0_0.O_ORDERKEY AS PROV_ORDERS_O__ORDERKEY
                          FROM
                            ORDERS F0_0
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
                (
                  (
                    (
                      (
                        (F0_0.S_SUPPKEY = F0_0.L_SUPPKEY)
                        AND (F0_0.PS_SUPPKEY = F0_0.L_SUPPKEY)
                      )
                      AND (F0_0.PS_PARTKEY = F0_0.L_PARTKEY)
                    )
                    AND (F0_0.P_PARTKEY = F0_0.L_PARTKEY)
                  )
                  AND (F0_0.O_ORDERKEY = F0_0.L_ORDERKEY)
                )
                AND (F0_0.S_NATIONKEY = F0_0.N_NATIONKEY)
              )
              AND (F0_0.P_NAME LIKE '%green%')
            )
        ) F1_0 ON (
          (
            (
              (F0_0."GROUP_1" = F1_0."_P_SIDE_GROUP_1")
              OR (
                (F0_0."GROUP_1" IS NULL)
                AND (F1_0."_P_SIDE_GROUP_1" IS NULL)
              )
            )
            AND (
              (F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")
              OR (
                (F0_0."GROUP_0" IS NULL)
                AND (F1_0."_P_SIDE_GROUP_0" IS NULL)
              )
            )
          )
        )
      )
    ORDER BY
      NATION ASC NULLS LAST,
      O_YEAR DESC NULLS LAST
  ) F0_0;
