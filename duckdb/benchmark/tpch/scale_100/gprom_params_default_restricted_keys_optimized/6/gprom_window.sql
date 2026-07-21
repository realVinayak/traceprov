WITH
  temp_view_2 AS (
    SELECT
      F0_0."AGGR_0" AS "AGGR_0" /* + materialize */,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      1 AS _RESULT_TID,
      ROW_NUMBER() OVER () AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          F0_0."AGG_GB_ARG0" AS "AGG_GB_ARG0",
          F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
          F0_0._RESULT_TID AS _RESULT_TID,
          F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT,
          SUM(F0_0."AGG_GB_ARG0") OVER () AS "AGGR_0",
          COUNT(1) OVER () AS __DUMMY_CNT
        FROM
          (
            (
              SELECT
                (F0_0.L_EXTENDEDPRICE * F0_0.L_DISCOUNT) AS "AGG_GB_ARG0",
                F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                F0_0._RESULT_TID AS _RESULT_TID,
                F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
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
                    F0_0.rowid AS PROV_LINEITEM_L__ORDERKEY,
                    F0_0.rowid AS _RESULT_TID,
                    1 AS _SETPROV_DUP_COUNT
                  FROM
                    LINEITEM AS F0_0
                ) AS F0_0
              WHERE
                (
                  (
                    (
                      (
                        (F0_0.L_SHIPDATE >= '1994-01-01')
                        AND (F0_0.L_SHIPDATE < '1995-01-01')
                      )
                      AND (F0_0.L_DISCOUNT >= 0.050000)
                    )
                    AND (F0_0.L_DISCOUNT <= 0.070000)
                  )
                  AND (F0_0.L_QUANTITY < 24)
                )
              UNION ALL
              (
                SELECT
                  NULL AS "AGG_GB_ARG0",
                  NULL AS PROV_LINEITEM_L__ORDERKEY,
                  -1 AS _RESULT_TID,
                  NULL AS _SETPROV_DUP_COUNT
              )
            )
          ) AS F0_0
      ) AS F0_0
    WHERE
      (
        (F0_0.__DUMMY_CNT = 1)
        OR (F0_0._RESULT_TID <> -1)
      )
  ),
  temp_view_1 AS (
    SELECT
      F0_0."AGGR_0" AS REVENUE /* + materialize */,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F0_0._RESULT_TID AS _RESULT_TID,
      F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          *
        FROM
          temp_view_2
      ) AS F0_0
  ),
  temp_view_0 AS (
    SELECT
      F0_0.REVENUE AS REVENUE /* + materialize */,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
    FROM
      (
        SELECT
          *
        FROM
          temp_view_1
      ) AS F0_0
  )
SELECT
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
FROM
  (
    SELECT
      *
    FROM
      temp_view_0
  ) AS F0_0
