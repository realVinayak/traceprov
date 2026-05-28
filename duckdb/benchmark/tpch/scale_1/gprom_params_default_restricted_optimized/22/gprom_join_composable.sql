WITH
  temp_view_3 AS (
    SELECT
      AVG(F0_1.C_ACCTBAL) AS "AVG(C_ACCTBAL)" /* + materialize */
    FROM
      CUSTOMER AS F0_1
    WHERE
      (
        (F0_1.C_ACCTBAL > 0.000000)
        AND SUBSTRING(F0_1.C_PHONE, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17')
      )
  ),
  temp_view_2 AS (
    SELECT
      SUBSTRING(F0_0.C_PHONE, 1, 2) AS CNTRYCODE /* + materialize */,
      F0_0.C_ACCTBAL AS C_ACCTBAL
    FROM
      CUSTOMER AS F0_0,
      LATERAL (
        SELECT
          F0_1."AVG(C_ACCTBAL)" AS "NESTING_EVAL_1"
        FROM
          (
            SELECT
              *
            FROM
              temp_view_3
          ) AS F0_1
      ) AS F1_0,
      LATERAL (
        SELECT
          (COUNT(1) > 0) AS "NESTING_EVAL_2"
        FROM
          ORDERS AS F0_1
        WHERE
          (F0_1.O_CUSTKEY = F0_0.C_CUSTKEY)
      ) AS F2_0
    WHERE
      (
        (
          SUBSTRING(F0_0.C_PHONE, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17')
          AND (F0_0.C_ACCTBAL > F1_0."NESTING_EVAL_1")
        )
        AND (NOT (F2_0."NESTING_EVAL_2"))
      )
  ),
  temp_view_9 AS (
    SELECT
      F0_0.C_CUSTKEY AS C_CUSTKEY /* + materialize */,
      F0_0.C_NAME AS C_NAME,
      F0_0.C_ADDRESS AS C_ADDRESS,
      F0_0.C_NATIONKEY AS C_NATIONKEY,
      F0_0.C_PHONE AS C_PHONE,
      F0_0.C_ACCTBAL AS C_ACCTBAL,
      F0_0.C_MKTSEGMENT AS C_MKTSEGMENT,
      F0_0.C_COMMENT AS C_COMMENT,
      F0_0.rowid AS PROV_CUSTOMER_C__CUSTKEY,
      F0_0.rowid AS _RESULT_TID,
      1 AS _SETPROV_DUP_COUNT
    FROM
      CUSTOMER AS F0_0
  ),
  temp_view_12 AS (
    SELECT
      F0_1."AGGR_0" AS "AGGR_0" /* + materialize */,
      F1_1."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
      1 AS _RESULT_TID,
      ROW_NUMBER() OVER () AS _SETPROV_DUP_COUNT
    FROM
      (
        (
          SELECT
            AVG(F0_1.C_ACCTBAL) AS "AGGR_0"
          FROM
            CUSTOMER AS F0_1
          WHERE
            (
              (F0_1.C_ACCTBAL > 0.000000)
              AND SUBSTRING(F0_1.C_PHONE, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17')
            )
        ) AS F0_1
        LEFT OUTER JOIN (
          SELECT
            F0_1.C_CUSTKEY AS C_CUSTKEY,
            F0_1.C_NAME AS C_NAME,
            F0_1.C_ADDRESS AS C_ADDRESS,
            F0_1.C_NATIONKEY AS C_NATIONKEY,
            F0_1.C_PHONE AS C_PHONE,
            F0_1.C_ACCTBAL AS C_ACCTBAL,
            F0_1.C_MKTSEGMENT AS C_MKTSEGMENT,
            F0_1.C_COMMENT AS C_COMMENT,
            F0_1."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
            F0_1._RESULT_TID AS _RESULT_TID,
            F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
          FROM
            (
              SELECT
                F0_1.C_CUSTKEY AS C_CUSTKEY,
                F0_1.C_NAME AS C_NAME,
                F0_1.C_ADDRESS AS C_ADDRESS,
                F0_1.C_NATIONKEY AS C_NATIONKEY,
                F0_1.C_PHONE AS C_PHONE,
                F0_1.C_ACCTBAL AS C_ACCTBAL,
                F0_1.C_MKTSEGMENT AS C_MKTSEGMENT,
                F0_1.C_COMMENT AS C_COMMENT,
                F0_1.rowid AS PROV_CUSTOMER_1_C__CUSTKEY,
                F0_1.rowid AS _RESULT_TID,
                1 AS _SETPROV_DUP_COUNT
              FROM
                CUSTOMER AS F0_1
            ) AS F0_1
          WHERE
            (
              (F0_1.C_ACCTBAL > 0.000000)
              AND SUBSTRING(F0_1.C_PHONE, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17')
            )
        ) AS F1_1 ON ((1 = 1))
      )
  ),
  temp_view_11 AS (
    SELECT
      F0_1."AGGR_0" AS "AVG(C_ACCTBAL)" /* + materialize */,
      F0_1."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
      F0_1._RESULT_TID AS _RESULT_TID,
      F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          *
        FROM
          temp_view_12
      ) AS F0_1
  ),
  temp_view_10 AS (
    SELECT
      F0_1."AVG(C_ACCTBAL)" AS "NESTING_EVAL_1" /* + materialize */,
      F0_1."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
      F0_1._RESULT_TID AS _RESULT_TID,
      F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          *
        FROM
          temp_view_11
      ) AS F0_1
  ),
  temp_view_8 AS (
    SELECT
      F0_0.C_CUSTKEY AS C_CUSTKEY /* + materialize */,
      F0_0.C_NAME AS C_NAME,
      F0_0.C_ADDRESS AS C_ADDRESS,
      F0_0.C_NATIONKEY AS C_NATIONKEY,
      F0_0.C_PHONE AS C_PHONE,
      F0_0.C_ACCTBAL AS C_ACCTBAL,
      F0_0.C_MKTSEGMENT AS C_MKTSEGMENT,
      F0_0.C_COMMENT AS C_COMMENT,
      F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F0_0.left__RESULT_TID AS left__RESULT_TID,
      F0_0.left__SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT,
      F1_0."NESTING_EVAL_1" AS "NESTING_EVAL_1",
      F1_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
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
          F0_0.C_CUSTKEY AS C_CUSTKEY,
          F0_0.C_NAME AS C_NAME,
          F0_0.C_ADDRESS AS C_ADDRESS,
          F0_0.C_NATIONKEY AS C_NATIONKEY,
          F0_0.C_PHONE AS C_PHONE,
          F0_0.C_ACCTBAL AS C_ACCTBAL,
          F0_0.C_MKTSEGMENT AS C_MKTSEGMENT,
          F0_0.C_COMMENT AS C_COMMENT,
          F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
          F0_0._RESULT_TID AS left__RESULT_TID,
          F0_0._SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              *
            FROM
              temp_view_9
          ) AS F0_0
      ) AS F0_0,
      LATERAL (
        SELECT
          F0_1."NESTING_EVAL_1" AS "NESTING_EVAL_1",
          F0_1."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
          F0_1._RESULT_TID AS right__RESULT_TID,
          F0_1._SETPROV_DUP_COUNT AS right__SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              *
            FROM
              temp_view_10
          ) AS F0_1
      ) AS F1_0
  ),
  temp_view_7 AS (
    SELECT
      F0_0.C_CUSTKEY AS C_CUSTKEY /* + materialize */,
      F0_0.C_NAME AS C_NAME,
      F0_0.C_ADDRESS AS C_ADDRESS,
      F0_0.C_NATIONKEY AS C_NATIONKEY,
      F0_0.C_PHONE AS C_PHONE,
      F0_0.C_ACCTBAL AS C_ACCTBAL,
      F0_0.C_MKTSEGMENT AS C_MKTSEGMENT,
      F0_0.C_COMMENT AS C_COMMENT,
      F0_0."NESTING_EVAL_1" AS "NESTING_EVAL_1",
      F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F0_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
      F0_0._RESULT_TID AS _RESULT_TID,
      F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          *
        FROM
          temp_view_8
      ) AS F0_0
  ),
  temp_view_6 AS (
    SELECT
      F0_0.C_CUSTKEY AS C_CUSTKEY /* + materialize */,
      F0_0.C_NAME AS C_NAME,
      F0_0.C_ADDRESS AS C_ADDRESS,
      F0_0.C_NATIONKEY AS C_NATIONKEY,
      F0_0.C_PHONE AS C_PHONE,
      F0_0.C_ACCTBAL AS C_ACCTBAL,
      F0_0.C_MKTSEGMENT AS C_MKTSEGMENT,
      F0_0.C_COMMENT AS C_COMMENT,
      F0_0."NESTING_EVAL_1" AS "NESTING_EVAL_1",
      F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F0_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
      F0_0.left__RESULT_TID AS left__RESULT_TID,
      F0_0.left__SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT,
      F1_0."NESTING_EVAL_2" AS "NESTING_EVAL_2",
      F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
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
          F0_0.C_CUSTKEY AS C_CUSTKEY,
          F0_0.C_NAME AS C_NAME,
          F0_0.C_ADDRESS AS C_ADDRESS,
          F0_0.C_NATIONKEY AS C_NATIONKEY,
          F0_0.C_PHONE AS C_PHONE,
          F0_0.C_ACCTBAL AS C_ACCTBAL,
          F0_0.C_MKTSEGMENT AS C_MKTSEGMENT,
          F0_0.C_COMMENT AS C_COMMENT,
          F0_0."NESTING_EVAL_1" AS "NESTING_EVAL_1",
          F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
          F0_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
          F0_0._RESULT_TID AS left__RESULT_TID,
          F0_0._SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              *
            FROM
              temp_view_7
          ) AS F0_0
      ) AS F0_0,
      LATERAL (
        SELECT
          F0_1."NESTING_EVAL_2" AS "NESTING_EVAL_2",
          F0_1.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
          F0_1._RESULT_TID AS right__RESULT_TID,
          F0_1._SETPROV_DUP_COUNT AS right__SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              (F0_1."AGGR_0" > 0) AS "NESTING_EVAL_2" /* + materialize */,
              F0_1.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
              F0_1._RESULT_TID AS _RESULT_TID,
              F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
            FROM
              (
                SELECT
                  F0_1."AGGR_0" AS "AGGR_0" /* + materialize */,
                  F1_1.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                  1 AS _RESULT_TID,
                  ROW_NUMBER() OVER () AS _SETPROV_DUP_COUNT
                FROM
                  (
                    (
                      SELECT
                        COUNT(1) AS "AGGR_0"
                      FROM
                        ORDERS AS F0_1
                      WHERE
                        (F0_1.O_CUSTKEY = F0_0.C_CUSTKEY)
                    ) AS F0_1
                    LEFT OUTER JOIN (
                      SELECT
                        F0_1.O_ORDERKEY AS O_ORDERKEY,
                        F0_1.O_CUSTKEY AS O_CUSTKEY,
                        F0_1.O_ORDERSTATUS AS O_ORDERSTATUS,
                        F0_1.O_TOTALPRICE AS O_TOTALPRICE,
                        F0_1.O_ORDERDATE AS O_ORDERDATE,
                        F0_1.O_ORDERPRIORITY AS O_ORDERPRIORITY,
                        F0_1.O_CLERK AS O_CLERK,
                        F0_1.O_SHIPPRIORITY AS O_SHIPPRIORITY,
                        F0_1.O_COMMENT AS O_COMMENT,
                        F0_1.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                        F0_1._RESULT_TID AS _RESULT_TID,
                        F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
                      FROM
                        (
                          SELECT
                            F0_1.O_ORDERKEY AS O_ORDERKEY,
                            F0_1.O_CUSTKEY AS O_CUSTKEY,
                            F0_1.O_ORDERSTATUS AS O_ORDERSTATUS,
                            F0_1.O_TOTALPRICE AS O_TOTALPRICE,
                            F0_1.O_ORDERDATE AS O_ORDERDATE,
                            F0_1.O_ORDERPRIORITY AS O_ORDERPRIORITY,
                            F0_1.O_CLERK AS O_CLERK,
                            F0_1.O_SHIPPRIORITY AS O_SHIPPRIORITY,
                            F0_1.O_COMMENT AS O_COMMENT,
                            F0_1.rowid AS PROV_ORDERS_O__ORDERKEY,
                            F0_1.rowid AS _RESULT_TID,
                            1 AS _SETPROV_DUP_COUNT
                          FROM
                            ORDERS AS F0_1
                        ) AS F0_1
                      WHERE
                        (F0_1.O_CUSTKEY = F0_0.C_CUSTKEY)
                    ) AS F1_1 ON ((1 = 1))
                  )
              ) AS F0_1
          ) AS F0_1
      ) AS F1_0
  ),
  temp_view_5 AS (
    SELECT
      SUBSTRING(F0_0.C_PHONE, 1, 2) AS CNTRYCODE /* + materialize */,
      F0_0.C_ACCTBAL AS C_ACCTBAL,
      F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F0_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F0_0._RESULT_TID AS _RESULT_TID,
      F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          F0_0.C_CUSTKEY AS C_CUSTKEY,
          F0_0.C_NAME AS C_NAME,
          F0_0.C_ADDRESS AS C_ADDRESS,
          F0_0.C_NATIONKEY AS C_NATIONKEY,
          F0_0.C_PHONE AS C_PHONE,
          F0_0.C_ACCTBAL AS C_ACCTBAL,
          F0_0.C_MKTSEGMENT AS C_MKTSEGMENT,
          F0_0.C_COMMENT AS C_COMMENT,
          F0_0."NESTING_EVAL_1" AS "NESTING_EVAL_1",
          F0_0."NESTING_EVAL_2" AS "NESTING_EVAL_2",
          F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
          F0_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
          F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
          F0_0._RESULT_TID AS _RESULT_TID,
          F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              *
            FROM
              temp_view_6
          ) AS F0_0
      ) AS F0_0
    WHERE
      (
        (
          SUBSTRING(F0_0.C_PHONE, 1, 2) IN ('13', '31', '23', '29', '30', '18', '17')
          AND (F0_0.C_ACCTBAL > F0_0."NESTING_EVAL_1")
        )
        AND (NOT (F0_0."NESTING_EVAL_2"))
      )
  ),
  temp_view_4 AS (
    SELECT
      1 AS "AGG_GB_ARG0" /* + materialize */,
      F0_0.C_ACCTBAL AS "AGG_GB_ARG1",
      F0_0.CNTRYCODE AS "AGG_GB_ARG2",
      F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F0_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
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
  temp_view_1 AS (
    SELECT
      F0_0."AGGR_0" AS "AGGR_0" /* + materialize */,
      F0_0."AGGR_1" AS "AGGR_1",
      F0_0."GROUP_0" AS "GROUP_0",
      F1_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F1_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
      F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      DENSE_RANK () OVER (
        ORDER BY
          F0_0."GROUP_0"
      ) AS _RESULT_TID,
      ROW_NUMBER() OVER (
        PARTITION BY
          F0_0."GROUP_0"
        ORDER BY
          F0_0."GROUP_0"
      ) AS _SETPROV_DUP_COUNT
    FROM
      (
        (
          SELECT
            COUNT(1) AS "AGGR_0",
            SUM(F0_0.C_ACCTBAL) AS "AGGR_1",
            F0_0.CNTRYCODE AS "GROUP_0"
          FROM
            (
              SELECT
                *
              FROM
                temp_view_2
            ) AS F0_0
          GROUP BY
            F0_0.CNTRYCODE
        ) AS F0_0
        JOIN (
          SELECT
            F0_0."AGG_GB_ARG2" AS "_P_SIDE_GROUP_0",
            F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
            F0_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
            F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY
          FROM
            (
              SELECT
                *
              FROM
                temp_view_4
            ) AS F0_0
        ) AS F1_0 ON (
          (
            (F0_0."GROUP_0" = F1_0."_P_SIDE_GROUP_0")
            OR (
              (F0_0."GROUP_0" IS NULL)
              AND (F1_0."_P_SIDE_GROUP_0" IS NULL)
            )
          )
        )
      )
  ),
  temp_view_0 AS (
    SELECT
      F0_0.CNTRYCODE AS CNTRYCODE /* + materialize */,
      F0_0.NUMCUST AS NUMCUST,
      F0_0.TOTACCTBAL AS TOTACCTBAL,
      F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F0_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY
    FROM
      (
        SELECT
          F0_0."GROUP_0" AS CNTRYCODE,
          F0_0."AGGR_0" AS NUMCUST,
          F0_0."AGGR_1" AS TOTACCTBAL,
          F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
          F0_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY",
          F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
          F0_0._RESULT_TID AS _RESULT_TID,
          F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              *
            FROM
              temp_view_1
          ) AS F0_0
        ORDER BY
          CNTRYCODE ASC
      ) AS F0_0
  )
SELECT
  F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
  F0_0."PROV_CUSTOMER_1_C__CUSTKEY" AS "PROV_CUSTOMER_1_C__CUSTKEY"
FROM
  (
    SELECT
      *
    FROM
      temp_view_0
  ) AS F0_0
