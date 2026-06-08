WITH
  temp_view_1 AS (
    /* + materialize */
    SELECT
      F0_0.L_QUANTITY AS "AGG_GB_ARG0",
      F0_0.L_EXTENDEDPRICE AS "AGG_GB_ARG1",
      (F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT)) AS "AGG_GB_ARG2",
      (
        (F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT)) * (1 + F0_0.L_TAX)
      ) AS "AGG_GB_ARG3",
      F0_0.L_QUANTITY AS "AGG_GB_ARG4",
      F0_0.L_EXTENDEDPRICE AS "AGG_GB_ARG5",
      F0_0.L_DISCOUNT AS "AGG_GB_ARG6",
      1 AS "AGG_GB_ARG7",
      F0_0.L_RETURNFLAG AS "AGG_GB_ARG8",
      F0_0.L_LINESTATUS AS "AGG_GB_ARG9",
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
    FROM
      (
        SELECT
          F0_0.L_ORDERKEY AS L_ORDERKEY,
          F0_0.L_PARTKEY AS L_PARTKEY,
          F0_0.L_SUPPKEY AS L_SUPPKEY,
          F0_0.L_LINENUMBER AS L_LINENUMBER,
          F0_0.L_QUANTITY AS L_QUANTITY,
          F0_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
          F0_0.L_DISCOUNT AS L_DISCOUNT,
          F0_0.L_TAX AS L_TAX,
          F0_0.L_RETURNFLAG AS L_RETURNFLAG,
          F0_0.L_LINESTATUS AS L_LINESTATUS,
          F0_0.L_SHIPDATE AS L_SHIPDATE,
          F0_0.L_COMMITDATE AS L_COMMITDATE,
          F0_0.L_RECEIPTDATE AS L_RECEIPTDATE,
          F0_0.L_SHIPINSTRUCT AS L_SHIPINSTRUCT,
          F0_0.L_SHIPMODE AS L_SHIPMODE,
          F0_0.L_COMMENT AS L_COMMENT,
          F0_0.rowid AS PROV_LINEITEM_L__ORDERKEY
        FROM
          LINEITEM AS F0_0
      ) AS F0_0
    WHERE
      (F0_0.L_SHIPDATE <= '1998-09-02')
  ),
  temp_view_0 AS (
    /* + materialize */
    SELECT
      F0_0."AGGR_0" AS "AGGR_0",
      F0_0."AGGR_1" AS "AGGR_1",
      F0_0."AGGR_2" AS "AGGR_2",
      F0_0."AGGR_3" AS "AGGR_3",
      F0_0."AGGR_4" AS "AGGR_4",
      F0_0."AGGR_5" AS "AGGR_5",
      F0_0."AGGR_6" AS "AGGR_6",
      F0_0."AGGR_7" AS "AGGR_7",
      F0_0."GROUP_0" AS "GROUP_0",
      F0_0."GROUP_1" AS "GROUP_1",
      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
    FROM
      (
        (
          SELECT
            SUM(F0_0.L_QUANTITY) AS "AGGR_0",
            SUM(F0_0.L_EXTENDEDPRICE) AS "AGGR_1",
            SUM((F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT))) AS "AGGR_2",
            SUM(
              (
                (F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT)) * (1 + F0_0.L_TAX)
              )
            ) AS "AGGR_3",
            AVG(F0_0.L_QUANTITY) AS "AGGR_4",
            AVG(F0_0.L_EXTENDEDPRICE) AS "AGGR_5",
            AVG(F0_0.L_DISCOUNT) AS "AGGR_6",
            COUNT(1) AS "AGGR_7",
            F0_0.L_RETURNFLAG AS "GROUP_0",
            F0_0.L_LINESTATUS AS "GROUP_1"
          FROM
            LINEITEM AS F0_0
          WHERE
            (F0_0.L_SHIPDATE <= '1998-09-02')
          GROUP BY
            F0_0.L_RETURNFLAG,
            F0_0.L_LINESTATUS
        ) AS F0_0
        JOIN (
          SELECT
            F0_0."AGG_GB_ARG8" AS "_P_SIDE_GROUP_0",
            F0_0."AGG_GB_ARG9" AS "_P_SIDE_GROUP_1",
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
          FROM
            (
              SELECT
                *
              FROM
                temp_view_1
            ) AS F0_0
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
  )
SELECT
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
FROM
  (
    SELECT
      F0_0."GROUP_0" AS L_RETURNFLAG,
      F0_0."GROUP_1" AS L_LINESTATUS,
      F0_0."AGGR_0" AS SUM_QTY,
      F0_0."AGGR_1" AS SUM_BASE_PRICE,
      F0_0."AGGR_2" AS SUM_DISC_PRICE,
      F0_0."AGGR_3" AS SUM_CHARGE,
      F0_0."AGGR_4" AS AVG_QTY,
      F0_0."AGGR_5" AS AVG_PRICE,
      F0_0."AGGR_6" AS AVG_DISC,
      F0_0."AGGR_7" AS COUNT_ORDER,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
    FROM
      (
        SELECT
          *
        FROM
          temp_view_0
      ) AS F0_0
    ORDER BY
      L_RETURNFLAG ASC,
      L_LINESTATUS ASC
  ) AS F0_0
