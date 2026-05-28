WITH
  temp_view_2 AS (
    SELECT
      /*+ materialize */ (
        CASE
          WHEN (
            (F0_0.O_ORDERPRIORITY = '1-URGENT')
            OR (F0_0.O_ORDERPRIORITY = '2-HIGH')
          ) THEN 1
          ELSE 0
        END
      ) AS "AGG_GB_ARG0",
      (
        CASE
          WHEN (
            (F0_0.O_ORDERPRIORITY <> '1-URGENT')
            AND (F0_0.O_ORDERPRIORITY <> '2-HIGH')
          ) THEN 1
          ELSE 0
        END
      ) AS "AGG_GB_ARG1",
      F0_0.L_SHIPMODE AS "AGG_GB_ARG2",
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
          F1_0.L_ORDERKEY AS L_ORDERKEY,
          F1_0.L_PARTKEY AS L_PARTKEY,
          F1_0.L_SUPPKEY AS L_SUPPKEY,
          F1_0.L_LINENUMBER AS L_LINENUMBER,
          F1_0.L_QUANTITY AS L_QUANTITY,
          F1_0.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
          F1_0.L_DISCOUNT AS L_DISCOUNT,
          F1_0.L_TAX AS L_TAX,
          F1_0.L_RETURNFLAG AS L_RETURNFLAG,
          F1_0.L_LINESTATUS AS L_LINESTATUS,
          F1_0.L_SHIPDATE AS L_SHIPDATE,
          F1_0.L_COMMITDATE AS L_COMMITDATE,
          F1_0.L_RECEIPTDATE AS L_RECEIPTDATE,
          F1_0.L_SHIPINSTRUCT AS L_SHIPINSTRUCT,
          F1_0.L_SHIPMODE AS L_SHIPMODE,
          F1_0.L_COMMENT AS L_COMMENT,
          F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
          F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
          hash(F0_0._RESULT_TID, F1_0._RESULT_TID) AS _RESULT_TID,
          GREATEST(F0_0._SETPROV_DUP_COUNT, F1_0._SETPROV_DUP_COUNT) AS _SETPROV_DUP_COUNT
        FROM
          (
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
                F0_0.O_ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                F0_0.rowid AS _RESULT_TID,
                1 AS _SETPROV_DUP_COUNT
              FROM
                ORDERS F0_0
            ) F0_0
            CROSS JOIN (
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
                F0_0.L_ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                F0_0.rowid AS _RESULT_TID,
                1 AS _SETPROV_DUP_COUNT
              FROM
                LINEITEM F0_0
            ) F1_0
          )
      ) F0_0
    WHERE
      (
        (
          (
            (
              (
                (F0_0.O_ORDERKEY = F0_0.L_ORDERKEY)
                AND F0_0.L_SHIPMODE IN ('MAIL', 'SHIP')
              )
              AND (F0_0.L_COMMITDATE < F0_0.L_RECEIPTDATE)
            )
            AND (F0_0.L_SHIPDATE < F0_0.L_COMMITDATE)
          )
          AND (F0_0.L_RECEIPTDATE >= '1994-01-01')
        )
        AND (F0_0.L_RECEIPTDATE < '1995-01-01')
      )
  ),
  temp_view_1 AS (
    SELECT
      /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0",
      F0_0."AGGR_1" AS "AGGR_1",
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
            SUM(
              (
                CASE
                  WHEN (
                    (F0_0.O_ORDERPRIORITY = '1-URGENT')
                    OR (F0_0.O_ORDERPRIORITY = '2-HIGH')
                  ) THEN 1
                  ELSE 0
                END
              )
            ) AS "AGGR_0",
            SUM(
              (
                CASE
                  WHEN (
                    (F0_0.O_ORDERPRIORITY <> '1-URGENT')
                    AND (F0_0.O_ORDERPRIORITY <> '2-HIGH')
                  ) THEN 1
                  ELSE 0
                END
              )
            ) AS "AGGR_1",
            F1_0.L_SHIPMODE AS "GROUP_0"
          FROM
            (
              ORDERS F0_0
              CROSS JOIN LINEITEM F1_0
            )
          WHERE
            (
              (
                (
                  (
                    (
                      (F0_0.O_ORDERKEY = F1_0.L_ORDERKEY)
                      AND F1_0.L_SHIPMODE IN ('MAIL', 'SHIP')
                    )
                    AND (F1_0.L_COMMITDATE < F1_0.L_RECEIPTDATE)
                  )
                  AND (F1_0.L_SHIPDATE < F1_0.L_COMMITDATE)
                )
                AND (F1_0.L_RECEIPTDATE >= '1994-01-01')
              )
              AND (F1_0.L_RECEIPTDATE < '1995-01-01')
            )
          GROUP BY
            F1_0.L_SHIPMODE
        ) F0_0
        JOIN (
          SELECT
            F0_0."AGG_GB_ARG2" AS "_P_SIDE_GROUP_0",
            F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
          FROM
            (
              SELECT
                *
              FROM
                temp_view_2
            ) F0_0
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
  ),
  temp_view_0 AS (
    SELECT
      /*+ materialize */ F0_0.L_SHIPMODE AS L_SHIPMODE,
      F0_0.HIGH_LINE_COUNT AS HIGH_LINE_COUNT,
      F0_0.LOW_LINE_COUNT AS LOW_LINE_COUNT,
      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
    FROM
      (
        SELECT
          F0_0."GROUP_0" AS L_SHIPMODE,
          F0_0."AGGR_0" AS HIGH_LINE_COUNT,
          F0_0."AGGR_1" AS LOW_LINE_COUNT,
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
          ) F0_0
        ORDER BY
          L_SHIPMODE ASC NULLS LAST
      ) F0_0
  )
SELECT
  F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F0_0.L_SHIPMODE AS L_SHIPMODE
FROM
  (
    SELECT
      *
    FROM
      temp_view_0
  ) F0_0;
