SELECT
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
FROM
  (
    SELECT
      F0_0."GROUP_0" AS L_RETURNFLAG,
      F0_0."GROUP_1" AS L_LINESTATUS,
      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
    FROM
      (
        (
          SELECT
            F0_0.L_RETURNFLAG AS "GROUP_0",
            F0_0.L_LINESTATUS AS "GROUP_1"
          FROM
            (
              SELECT
                F0_0.L_RETURNFLAG AS L_RETURNFLAG,
                F0_0.L_LINESTATUS AS L_LINESTATUS,
                F0_0.L_SHIPDATE AS L_SHIPDATE
              FROM
                LINEITEM AS F0_0
            ) AS F0_0
          WHERE
            (F0_0.L_SHIPDATE <= '1998-09-02')
          GROUP BY
            F0_0.L_RETURNFLAG,
            F0_0.L_LINESTATUS
        ) AS F0_0
        JOIN (
          SELECT
            F0_0.L_RETURNFLAG AS "_P_SIDE_GROUP_0",
            F0_0.L_LINESTATUS AS "_P_SIDE_GROUP_1",
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
          FROM
            (
              SELECT
                F0_0.L_RETURNFLAG AS L_RETURNFLAG,
                F0_0.L_LINESTATUS AS L_LINESTATUS,
                F0_0.L_SHIPDATE AS L_SHIPDATE,
                F0_0.rowid AS PROV_LINEITEM_L__ORDERKEY
              FROM
                LINEITEM AS F0_0
            ) AS F0_0
          WHERE
            (F0_0.L_SHIPDATE <= '1998-09-02')
        ) AS F1_0 ON (
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
      L_RETURNFLAG ASC,
      L_LINESTATUS ASC
  ) AS F0_0
