WITH
  temp_view_5 AS (
    /* + materialize */
    SELECT
      F0_0."AGGR_0" AS "AGGR_0",
      F0_0."AGG_GB_ARG1" AS "GROUP_0",
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      DENSE_RANK () OVER (
        ORDER BY
          F0_0."AGG_GB_ARG1"
      ) AS _RESULT_TID,
      ROW_NUMBER() OVER (
        PARTITION BY
          F0_0."AGG_GB_ARG1"
        ORDER BY
          F0_0."AGG_GB_ARG1"
      ) AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          (F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT)) AS "AGG_GB_ARG0",
          F0_0.L_SUPPKEY AS "AGG_GB_ARG1",
          F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
          F0_0._RESULT_TID AS _RESULT_TID,
          F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT,
          SUM((F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT))) OVER (
            PARTITION BY
              F0_0.L_SUPPKEY
          ) AS "AGGR_0"
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
            (F0_0.L_SHIPDATE >= CAST(('1996-01-01') AS DATE))
            AND (
              F0_0.L_SHIPDATE < (
                CAST(('1996-01-01') AS DATE) + CAST(('3 month') AS INTERVAL)
              )
            )
          )
      ) AS F0_0
  ),
  temp_view_4 AS (
    /* + materialize */
    SELECT
      F0_0."GROUP_0" AS SUPPLIER_NO,
      F0_0."AGGR_0" AS TOTAL_REVENUE,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F0_0._RESULT_TID AS _RESULT_TID,
      F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          *
        FROM
          temp_view_5
      ) AS F0_0
  ),
  temp_view_3 AS (
    /* + materialize */
    SELECT
      F0_0.S_SUPPKEY AS S_SUPPKEY,
      F0_0.S_NAME AS S_NAME,
      F0_0.S_ADDRESS AS S_ADDRESS,
      F0_0.S_NATIONKEY AS S_NATIONKEY,
      F0_0.S_PHONE AS S_PHONE,
      F0_0.S_ACCTBAL AS S_ACCTBAL,
      F0_0.S_COMMENT AS S_COMMENT,
      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F0_0._RESULT_TID AS left__RESULT_TID,
      F0_0._SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT,
      F1_0.SUPPLIER_NO AS SUPPLIER_NO,
      F1_0.TOTAL_REVENUE AS TOTAL_REVENUE,
      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F1_0._RESULT_TID AS right__RESULT_TID,
      F1_0._SETPROV_DUP_COUNT AS right__SETPROV_DUP_COUNT,
      HASH(F0_0._RESULT_TID, F1_0._RESULT_TID) AS _RESULT_TID,
      GREATEST(F0_0._SETPROV_DUP_COUNT, F1_0._SETPROV_DUP_COUNT) AS _SETPROV_DUP_COUNT
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
            F0_0.rowid AS PROV_SUPPLIER_S__SUPPKEY,
            F0_0.rowid AS _RESULT_TID,
            1 AS _SETPROV_DUP_COUNT
          FROM
            SUPPLIER AS F0_0
        ) AS F0_0
        CROSS JOIN (
          SELECT
            *
          FROM
            temp_view_4
        ) AS F1_0
      )
  ),
  temp_view_2 AS (
    /* + materialize */
    SELECT
      F0_0.S_SUPPKEY AS S_SUPPKEY,
      F0_0.S_NAME AS S_NAME,
      F0_0.S_ADDRESS AS S_ADDRESS,
      F0_0.S_NATIONKEY AS S_NATIONKEY,
      F0_0.S_PHONE AS S_PHONE,
      F0_0.S_ACCTBAL AS S_ACCTBAL,
      F0_0.S_COMMENT AS S_COMMENT,
      F0_0.SUPPLIER_NO AS SUPPLIER_NO,
      F0_0.TOTAL_REVENUE AS TOTAL_REVENUE,
      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F0_0._RESULT_TID AS _RESULT_TID,
      F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          *
        FROM
          temp_view_3
      ) AS F0_0
  ),
  temp_view_8 AS (
    /* + materialize */
    SELECT
      F0_1."AGGR_0" AS "AGGR_0",
      F0_1."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY",
      1 AS _RESULT_TID,
      ROW_NUMBER() OVER () AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          F0_1.SUPPLIER_NO AS SUPPLIER_NO,
          F0_1.TOTAL_REVENUE AS TOTAL_REVENUE,
          F0_1."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY",
          F0_1._RESULT_TID AS _RESULT_TID,
          F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT,
          MAX(
            (
              CASE
                WHEN (1 = F0_1._SETPROV_DUP_COUNT) THEN F0_1.TOTAL_REVENUE
                ELSE NULL
              END
            )
          ) OVER () AS "AGGR_0",
          COUNT(1) OVER () AS __DUMMY_CNT
        FROM
          (
            (
              SELECT
                F0_1.SUPPLIER_NO AS SUPPLIER_NO,
                F0_1.TOTAL_REVENUE AS TOTAL_REVENUE,
                F0_1.PROV_LINEITEM_L__ORDERKEY AS "PROV_LINEITEM_1_L__ORDERKEY",
                F0_1._RESULT_TID AS _RESULT_TID,
                F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
              FROM
                (
                  SELECT
                    *
                  FROM
                    temp_view_4
                ) AS F0_1
              UNION ALL
              (
                SELECT
                  NULL AS SUPPLIER_NO,
                  NULL AS TOTAL_REVENUE,
                  NULL AS "PROV_LINEITEM_1_L__ORDERKEY",
                  -1 AS _RESULT_TID,
                  NULL AS _SETPROV_DUP_COUNT
              )
            )
          ) AS F0_1
      ) AS F0_1
    WHERE
      (
        (F0_1.__DUMMY_CNT = 1)
        OR (F0_1._RESULT_TID <> -1)
      )
  ),
  temp_view_7 AS (
    /* + materialize */
    SELECT
      F0_1."AGGR_0" AS "MAX(TOTAL_REVENUE)",
      F0_1."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY",
      F0_1._RESULT_TID AS _RESULT_TID,
      F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          *
        FROM
          temp_view_8
      ) AS F0_1
  ),
  temp_view_6 AS (
    /* + materialize */
    SELECT
      F0_1."MAX(TOTAL_REVENUE)" AS "NESTING_EVAL_1",
      F0_1."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY",
      F0_1._RESULT_TID AS _RESULT_TID,
      F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          *
        FROM
          temp_view_7
      ) AS F0_1
  ),
  temp_view_1 AS (
    /* + materialize */
    SELECT
      F0_0.S_SUPPKEY AS S_SUPPKEY,
      F0_0.S_NAME AS S_NAME,
      F0_0.S_ADDRESS AS S_ADDRESS,
      F0_0.S_NATIONKEY AS S_NATIONKEY,
      F0_0.S_PHONE AS S_PHONE,
      F0_0.S_ACCTBAL AS S_ACCTBAL,
      F0_0.S_COMMENT AS S_COMMENT,
      F0_0.SUPPLIER_NO AS SUPPLIER_NO,
      F0_0.TOTAL_REVENUE AS TOTAL_REVENUE,
      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F0_0.left__RESULT_TID AS left__RESULT_TID,
      F0_0.left__SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT,
      F1_0."NESTING_EVAL_1" AS "NESTING_EVAL_1",
      F1_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY",
      F1_0.right__RESULT_TID AS right__RESULT_TID,
      F1_0.right__SETPROV_DUP_COUNT AS right__SETPROV_DUP_COUNT,
      HASH(F0_0.left__RESULT_TID, F1_0.right__RESULT_TID) AS _RESULT_TID,
      GREATEST(
        F0_0.left__SETPROV_DUP_COUNT,
        F1_0.right__SETPROV_DUP_COUNT
      ) AS _SETPROV_DUP_COUNT
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
          F0_0.SUPPLIER_NO AS SUPPLIER_NO,
          F0_0.TOTAL_REVENUE AS TOTAL_REVENUE,
          F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
          F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
          F0_0._RESULT_TID AS left__RESULT_TID,
          F0_0._SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              *
            FROM
              temp_view_2
          ) AS F0_0
      ) AS F0_0,
      LATERAL (
        SELECT
          F0_1."NESTING_EVAL_1" AS "NESTING_EVAL_1",
          F0_1."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY",
          F0_1._RESULT_TID AS right__RESULT_TID,
          F0_1._SETPROV_DUP_COUNT AS right__SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              *
            FROM
              temp_view_6
          ) AS F0_1
      ) AS F1_0
  ),
  temp_view_0 AS (
    /* + materialize */
    SELECT
      F0_0.S_SUPPKEY AS S_SUPPKEY,
      F0_0.S_NAME AS S_NAME,
      F0_0.S_ADDRESS AS S_ADDRESS,
      F0_0.S_PHONE AS S_PHONE,
      F0_0.TOTAL_REVENUE AS TOTAL_REVENUE,
      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F0_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY"
    FROM
      (
        SELECT
          F0_0.S_SUPPKEY AS S_SUPPKEY,
          F0_0.S_NAME AS S_NAME,
          F0_0.S_ADDRESS AS S_ADDRESS,
          F0_0.S_PHONE AS S_PHONE,
          F0_0.TOTAL_REVENUE AS TOTAL_REVENUE,
          F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
          F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
          F0_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY",
          F0_0._RESULT_TID AS _RESULT_TID,
          F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
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
              F0_0.SUPPLIER_NO AS SUPPLIER_NO,
              F0_0.TOTAL_REVENUE AS TOTAL_REVENUE,
              F0_0."NESTING_EVAL_1" AS "NESTING_EVAL_1",
              F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
              F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
              F0_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY",
              F0_0._RESULT_TID AS _RESULT_TID,
              F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
            FROM
              (
                SELECT
                  *
                FROM
                  temp_view_1
              ) AS F0_0
          ) AS F0_0
        WHERE
          (
            (F0_0.S_SUPPKEY = F0_0.SUPPLIER_NO)
            AND (F0_0.TOTAL_REVENUE = F0_0."NESTING_EVAL_1")
          )
        ORDER BY
          S_SUPPKEY ASC
      ) AS F0_0
  )
SELECT
  F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F0_0."PROV_LINEITEM_1_L__ORDERKEY" AS "PROV_LINEITEM_1_L__ORDERKEY",
  F0_0.S_SUPPKEY AS S_SUPPKEY,
  F0_0.S_NAME AS S_NAME,
  F0_0.S_ADDRESS AS S_ADDRESS,
  F0_0.S_PHONE AS S_PHONE
FROM
  (
    SELECT
      *
    FROM
      temp_view_0
  ) AS F0_0
