SELECT
  F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F1_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY
FROM
  (
    (
      SELECT
        COUNT(0) AS __DUMMY_EMPTY
      FROM
        (
          (
            SELECT
              F0_0.L_PARTKEY AS L_PARTKEY,
              F0_0.L_QUANTITY AS L_QUANTITY,
              F0_0.L_SHIPINSTRUCT AS L_SHIPINSTRUCT,
              F0_0.L_SHIPMODE AS L_SHIPMODE
            FROM
              LINEITEM F0_0
          ) F0_0
          CROSS JOIN (
            SELECT
              F0_0.P_PARTKEY AS P_PARTKEY,
              F0_0.P_BRAND AS P_BRAND,
              F0_0.P_SIZE AS P_SIZE,
              F0_0.P_CONTAINER AS P_CONTAINER
            FROM
              PART F0_0
          ) F1_0
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
                        (
                          (
                            (F1_0.P_PARTKEY = F0_0.L_PARTKEY)
                            AND (F1_0.P_BRAND = 'Brand#12')
                          )
                          AND F1_0.P_CONTAINER IN ('SM CASE', 'SM BOX', 'SM PACK', 'SM PKG')
                        )
                        AND (F0_0.L_QUANTITY >= 1)
                      )
                      AND (F0_0.L_QUANTITY <= (1 + 10))
                    )
                    AND (F1_0.P_SIZE >= 1)
                  )
                  AND (F1_0.P_SIZE <= 5)
                )
                AND F0_0.L_SHIPMODE IN ('AIR', 'AIR REG')
              )
              AND (F0_0.L_SHIPINSTRUCT = 'DELIVER IN PERSON')
            )
            OR (
              (
                (
                  (
                    (
                      (
                        (
                          (
                            (F1_0.P_PARTKEY = F0_0.L_PARTKEY)
                            AND (F1_0.P_BRAND = 'Brand#23')
                          )
                          AND F1_0.P_CONTAINER IN ('MED BAG', 'MED BOX', 'MED PKG', 'MED PACK')
                        )
                        AND (F0_0.L_QUANTITY >= 10)
                      )
                      AND (F0_0.L_QUANTITY <= (10 + 10))
                    )
                    AND (F1_0.P_SIZE >= 1)
                  )
                  AND (F1_0.P_SIZE <= 10)
                )
                AND F0_0.L_SHIPMODE IN ('AIR', 'AIR REG')
              )
              AND (F0_0.L_SHIPINSTRUCT = 'DELIVER IN PERSON')
            )
          )
          OR (
            (
              (
                (
                  (
                    (
                      (
                        (
                          (F1_0.P_PARTKEY = F0_0.L_PARTKEY)
                          AND (F1_0.P_BRAND = 'Brand#34')
                        )
                        AND F1_0.P_CONTAINER IN ('LG CASE', 'LG BOX', 'LG PACK', 'LG PKG')
                      )
                      AND (F0_0.L_QUANTITY >= 20)
                    )
                    AND (F0_0.L_QUANTITY <= (20 + 10))
                  )
                  AND (F1_0.P_SIZE >= 1)
                )
                AND (F1_0.P_SIZE <= 15)
              )
              AND F0_0.L_SHIPMODE IN ('AIR', 'AIR REG')
            )
            AND (F0_0.L_SHIPINSTRUCT = 'DELIVER IN PERSON')
          )
        )
    ) F0_0
    LEFT OUTER JOIN (
      SELECT
        F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
        F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY
      FROM
        (
          SELECT
            F0_0.L_PARTKEY AS L_PARTKEY,
            F0_0.L_QUANTITY AS L_QUANTITY,
            F0_0.L_SHIPINSTRUCT AS L_SHIPINSTRUCT,
            F0_0.L_SHIPMODE AS L_SHIPMODE,
            F1_0.P_PARTKEY AS P_PARTKEY,
            F1_0.P_BRAND AS P_BRAND,
            F1_0.P_SIZE AS P_SIZE,
            F1_0.P_CONTAINER AS P_CONTAINER,
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
            F1_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY
          FROM
            (
              (
                SELECT
                  F0_0.L_PARTKEY AS L_PARTKEY,
                  F0_0.L_QUANTITY AS L_QUANTITY,
                  F0_0.L_SHIPINSTRUCT AS L_SHIPINSTRUCT,
                  F0_0.L_SHIPMODE AS L_SHIPMODE,
                  F0_0.L_ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
                FROM
                  LINEITEM F0_0
              ) F0_0
              CROSS JOIN (
                SELECT
                  F0_0.P_PARTKEY AS P_PARTKEY,
                  F0_0.P_BRAND AS P_BRAND,
                  F0_0.P_SIZE AS P_SIZE,
                  F0_0.P_CONTAINER AS P_CONTAINER,
                  F0_0.P_PARTKEY AS PROV_PART_P__PARTKEY
                FROM
                  PART F0_0
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
                        (
                          (
                            (F0_0.P_PARTKEY = F0_0.L_PARTKEY)
                            AND (F0_0.P_BRAND = 'Brand#12')
                          )
                          AND F0_0.P_CONTAINER IN ('SM CASE', 'SM BOX', 'SM PACK', 'SM PKG')
                        )
                        AND (F0_0.L_QUANTITY >= 1)
                      )
                      AND (F0_0.L_QUANTITY <= (1 + 10))
                    )
                    AND (F0_0.P_SIZE >= 1)
                  )
                  AND (F0_0.P_SIZE <= 5)
                )
                AND F0_0.L_SHIPMODE IN ('AIR', 'AIR REG')
              )
              AND (F0_0.L_SHIPINSTRUCT = 'DELIVER IN PERSON')
            )
            OR (
              (
                (
                  (
                    (
                      (
                        (
                          (
                            (F0_0.P_PARTKEY = F0_0.L_PARTKEY)
                            AND (F0_0.P_BRAND = 'Brand#23')
                          )
                          AND F0_0.P_CONTAINER IN ('MED BAG', 'MED BOX', 'MED PKG', 'MED PACK')
                        )
                        AND (F0_0.L_QUANTITY >= 10)
                      )
                      AND (F0_0.L_QUANTITY <= (10 + 10))
                    )
                    AND (F0_0.P_SIZE >= 1)
                  )
                  AND (F0_0.P_SIZE <= 10)
                )
                AND F0_0.L_SHIPMODE IN ('AIR', 'AIR REG')
              )
              AND (F0_0.L_SHIPINSTRUCT = 'DELIVER IN PERSON')
            )
          )
          OR (
            (
              (
                (
                  (
                    (
                      (
                        (
                          (F0_0.P_PARTKEY = F0_0.L_PARTKEY)
                          AND (F0_0.P_BRAND = 'Brand#34')
                        )
                        AND F0_0.P_CONTAINER IN ('LG CASE', 'LG BOX', 'LG PACK', 'LG PKG')
                      )
                      AND (F0_0.L_QUANTITY >= 20)
                    )
                    AND (F0_0.L_QUANTITY <= (20 + 10))
                  )
                  AND (F0_0.P_SIZE >= 1)
                )
                AND (F0_0.P_SIZE <= 15)
              )
              AND F0_0.L_SHIPMODE IN ('AIR', 'AIR REG')
            )
            AND (F0_0.L_SHIPINSTRUCT = 'DELIVER IN PERSON')
          )
        )
    ) F1_0 ON ((1 = 1))
  );
