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
              F0_0.L_SHIPDATE AS L_SHIPDATE
            FROM
              LINEITEM F0_0
          ) F0_0
          CROSS JOIN (
            SELECT
              F0_0.P_PARTKEY AS P_PARTKEY
            FROM
              PART F0_0
          ) F1_0
        )
      WHERE
        (
          (
            (F0_0.L_PARTKEY = F1_0.P_PARTKEY)
            AND (F0_0.L_SHIPDATE >= '1995-09-01')
          )
          AND (F0_0.L_SHIPDATE < '1995-10-01')
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
            F0_0.L_SHIPDATE AS L_SHIPDATE,
            F1_0.P_PARTKEY AS P_PARTKEY,
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
            F1_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY
          FROM
            (
              (
                SELECT
                  F0_0.L_PARTKEY AS L_PARTKEY,
                  F0_0.L_SHIPDATE AS L_SHIPDATE,
                  F0_0.L_ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
                FROM
                  LINEITEM F0_0
              ) F0_0
              CROSS JOIN (
                SELECT
                  F0_0.P_PARTKEY AS P_PARTKEY,
                  F0_0.P_PARTKEY AS PROV_PART_P__PARTKEY
                FROM
                  PART F0_0
              ) F1_0
            )
        ) F0_0
      WHERE
        (
          (
            (F0_0.L_PARTKEY = F0_0.P_PARTKEY)
            AND (F0_0.L_SHIPDATE >= '1995-09-01')
          )
          AND (F0_0.L_SHIPDATE < '1995-10-01')
        )
    ) F1_0 ON ((1 = 1))
  );
