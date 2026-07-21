WITH
  temp_view_4 AS (
    /* + materialize */
    SELECT
      F0_0.O_ORDERKEY AS O_ORDERKEY,
      F0_0.O_CUSTKEY AS O_CUSTKEY,
      F0_0.O_ORDERSTATUS AS O_ORDERSTATUS,
      F0_0.O_TOTALPRICE AS O_TOTALPRICE,
      F0_0.O_ORDERDATE AS O_ORDERDATE,
      F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
      F0_0.O_CLERK AS O_CLERK,
      F0_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
      F0_0.O_COMMENT AS O_COMMENT,
      F0_0.rowid AS PROV_ORDERS_O__ORDERKEY,
      F0_0.rowid AS _RESULT_TID,
      1 AS _SETPROV_DUP_COUNT
    FROM
      ORDERS AS F0_0
  ),
  temp_view_3 AS (
    /* + materialize */
    SELECT
      F0_0.O_ORDERKEY AS O_ORDERKEY,
      F0_0.O_CUSTKEY AS O_CUSTKEY,
      F0_0.O_ORDERSTATUS AS O_ORDERSTATUS,
      F0_0.O_TOTALPRICE AS O_TOTALPRICE,
      F0_0.O_ORDERDATE AS O_ORDERDATE,
      F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
      F0_0.O_CLERK AS O_CLERK,
      F0_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
      F0_0.O_COMMENT AS O_COMMENT,
      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F0_0.left__RESULT_TID AS left__RESULT_TID,
      F0_0.left__SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT,
      F1_0."NESTING_EVAL_1" AS "NESTING_EVAL_1",
      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
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
          F0_0.O_ORDERKEY AS O_ORDERKEY,
          F0_0.O_CUSTKEY AS O_CUSTKEY,
          F0_0.O_ORDERSTATUS AS O_ORDERSTATUS,
          F0_0.O_TOTALPRICE AS O_TOTALPRICE,
          F0_0.O_ORDERDATE AS O_ORDERDATE,
          F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
          F0_0.O_CLERK AS O_CLERK,
          F0_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
          F0_0.O_COMMENT AS O_COMMENT,
          F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
          F0_0._RESULT_TID AS left__RESULT_TID,
          F0_0._SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              *
            FROM
              temp_view_4
          ) AS F0_0
      ) AS F0_0,
      LATERAL (
        SELECT
          F0_1."NESTING_EVAL_1" AS "NESTING_EVAL_1",
          F0_1.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
          F0_1._RESULT_TID AS right__RESULT_TID,
          F0_1._SETPROV_DUP_COUNT AS right__SETPROV_DUP_COUNT
        FROM
          (
            /* + materialize */
            SELECT
              (F0_1."AGGR_0" > 0) AS "NESTING_EVAL_1",
              F0_1.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
              F0_1._RESULT_TID AS _RESULT_TID,
              F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
            FROM
              (
                /* + materialize */
                SELECT
                  F0_1."AGGR_0" AS "AGGR_0",
                  F1_1.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                  1 AS _RESULT_TID,
                  ROW_NUMBER() OVER () AS _SETPROV_DUP_COUNT
                FROM
                  (
                    (
                      SELECT
                        COUNT(1) AS "AGGR_0"
                      FROM
                        LINEITEM AS F0_1
                      WHERE
                        (
                          (F0_1.L_ORDERKEY = F0_0.O_ORDERKEY)
                          AND (F0_1.L_COMMITDATE < F0_1.L_RECEIPTDATE)
                        )
                    ) AS F0_1
                    LEFT OUTER JOIN (
                      SELECT
                        F0_1.L_ORDERKEY AS L_ORDERKEY,
                        F0_1.L_PARTKEY AS L_PARTKEY,
                        F0_1.L_SUPPKEY AS L_SUPPKEY,
                        F0_1.L_LINENUMBER AS L_LINENUMBER,
                        F0_1.L_QUANTITY AS L_QUANTITY,
                        F0_1.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                        F0_1.L_DISCOUNT AS L_DISCOUNT,
                        F0_1.L_TAX AS L_TAX,
                        F0_1.L_RETURNFLAG AS L_RETURNFLAG,
                        F0_1.L_LINESTATUS AS L_LINESTATUS,
                        F0_1.L_SHIPDATE AS L_SHIPDATE,
                        F0_1.L_COMMITDATE AS L_COMMITDATE,
                        F0_1.L_RECEIPTDATE AS L_RECEIPTDATE,
                        F0_1.L_SHIPINSTRUCT AS L_SHIPINSTRUCT,
                        F0_1.L_SHIPMODE AS L_SHIPMODE,
                        F0_1.L_COMMENT AS L_COMMENT,
                        F0_1.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                        F0_1._RESULT_TID AS _RESULT_TID,
                        F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
                      FROM
                        (
                          SELECT
                            F0_1.L_ORDERKEY AS L_ORDERKEY,
                            F0_1.L_PARTKEY AS L_PARTKEY,
                            F0_1.L_SUPPKEY AS L_SUPPKEY,
                            F0_1.L_LINENUMBER AS L_LINENUMBER,
                            F0_1.L_QUANTITY AS L_QUANTITY,
                            F0_1.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                            F0_1.L_DISCOUNT AS L_DISCOUNT,
                            F0_1.L_TAX AS L_TAX,
                            F0_1.L_RETURNFLAG AS L_RETURNFLAG,
                            F0_1.L_LINESTATUS AS L_LINESTATUS,
                            F0_1.L_SHIPDATE AS L_SHIPDATE,
                            F0_1.L_COMMITDATE AS L_COMMITDATE,
                            F0_1.L_RECEIPTDATE AS L_RECEIPTDATE,
                            F0_1.L_SHIPINSTRUCT AS L_SHIPINSTRUCT,
                            F0_1.L_SHIPMODE AS L_SHIPMODE,
                            F0_1.L_COMMENT AS L_COMMENT,
                            F0_1.rowid AS PROV_LINEITEM_L__ORDERKEY,
                            F0_1.rowid AS _RESULT_TID,
                            1 AS _SETPROV_DUP_COUNT
                          FROM
                            LINEITEM AS F0_1
                        ) AS F0_1
                      WHERE
                        (
                          (F0_1.L_ORDERKEY = F0_0.O_ORDERKEY)
                          AND (F0_1.L_COMMITDATE < F0_1.L_RECEIPTDATE)
                        )
                    ) AS F1_1 ON ((1 = 1))
                  )
              ) AS F0_1
          ) AS F0_1
      ) AS F1_0
  ),
  temp_view_2 AS (
    /* + materialize */
    SELECT
      1 AS "AGG_GB_ARG0",
      F0_0.O_ORDERPRIORITY AS "AGG_GB_ARG1",
      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F0_0._RESULT_TID AS _RESULT_TID,
      F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          F0_0.O_ORDERKEY AS O_ORDERKEY,
          F0_0.O_CUSTKEY AS O_CUSTKEY,
          F0_0.O_ORDERSTATUS AS O_ORDERSTATUS,
          F0_0.O_TOTALPRICE AS O_TOTALPRICE,
          F0_0.O_ORDERDATE AS O_ORDERDATE,
          F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
          F0_0.O_CLERK AS O_CLERK,
          F0_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
          F0_0.O_COMMENT AS O_COMMENT,
          F0_0."NESTING_EVAL_1" AS "NESTING_EVAL_1",
          F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
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
      ) AS F0_0
    WHERE
      (
        (
          (F0_0.O_ORDERDATE >= '1993-07-01')
          AND (F0_0.O_ORDERDATE < '1993-10-01')
        )
        AND F0_0."NESTING_EVAL_1"
      )
  ),
  temp_view_1 AS (
    /* + materialize */
    SELECT
      F0_0."AGGR_0" AS "AGGR_0",
      F0_0."GROUP_0" AS "GROUP_0",
      F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
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
            F0_0.O_ORDERPRIORITY AS "GROUP_0"
          FROM
            ORDERS AS F0_0,
            LATERAL (
              SELECT
                (COUNT(1) > 0) AS "NESTING_EVAL_1"
              FROM
                LINEITEM AS F0_1
              WHERE
                (
                  (F0_1.L_ORDERKEY = F0_0.O_ORDERKEY)
                  AND (F0_1.L_COMMITDATE < F0_1.L_RECEIPTDATE)
                )
            ) AS F1_0
          WHERE
            (
              (
                (F0_0.O_ORDERDATE >= '1993-07-01')
                AND (F0_0.O_ORDERDATE < '1993-10-01')
              )
              AND F1_0."NESTING_EVAL_1"
            )
          GROUP BY
            F0_0.O_ORDERPRIORITY
        ) AS F0_0
        JOIN (
          SELECT
            F0_0."AGG_GB_ARG1" AS "_P_SIDE_GROUP_0",
            F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
          FROM
            (
              SELECT
                *
              FROM
                temp_view_2
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
    /* + materialize */
    SELECT
      F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
      F0_0.ORDER_COUNT AS ORDER_COUNT,
      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
    FROM
      (
        SELECT
          F0_0."GROUP_0" AS O_ORDERPRIORITY,
          F0_0."AGGR_0" AS ORDER_COUNT,
          F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
          F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
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
          O_ORDERPRIORITY ASC
      ) AS F0_0
  )
SELECT
  F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY
FROM
  (
    SELECT
      *
    FROM
      temp_view_0
  ) AS F0_0
