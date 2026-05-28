WITH
  temp_view_2 AS (
    SELECT
      (F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT)) AS "AGG_GB_ARG0" /* + materialize */,
      F0_0.L_ORDERKEY AS "AGG_GB_ARG1",
      F0_0.O_ORDERDATE AS "AGG_GB_ARG2",
      F0_0.O_SHIPPRIORITY AS "AGG_GB_ARG3",
      F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
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
          F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
          F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
          F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
        FROM
          (
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
                F1_0.O_ORDERKEY AS O_ORDERKEY,
                F1_0.O_CUSTKEY AS O_CUSTKEY,
                F1_0.O_ORDERSTATUS AS O_ORDERSTATUS,
                F1_0.O_TOTALPRICE AS O_TOTALPRICE,
                F1_0.O_ORDERDATE AS O_ORDERDATE,
                F1_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
                F1_0.O_CLERK AS O_CLERK,
                F1_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
                F1_0.O_COMMENT AS O_COMMENT,
                F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY
              FROM
                (
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
                      F0_0.rowid AS PROV_CUSTOMER_C__CUSTKEY
                    FROM
                      CUSTOMER AS F0_0
                  ) AS F0_0
                  CROSS JOIN (
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
                      F0_0.rowid AS PROV_ORDERS_O__ORDERKEY
                    FROM
                      ORDERS AS F0_0
                  ) AS F1_0
                )
            ) AS F0_0
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
                F0_0.rowid AS PROV_LINEITEM_L__ORDERKEY
              FROM
                LINEITEM AS F0_0
            ) AS F1_0
          )
      ) AS F0_0
    WHERE
      (
        (
          (
            (
              (F0_0.C_MKTSEGMENT = 'BUILDING')
              AND (F0_0.C_CUSTKEY = F0_0.O_CUSTKEY)
            )
            AND (F0_0.L_ORDERKEY = F0_0.O_ORDERKEY)
          )
          AND (F0_0.O_ORDERDATE < '1995-03-15')
        )
        AND (F0_0.L_SHIPDATE > '1995-03-15')
      )
  ),
  temp_view_1 AS (
    SELECT
      F0_0."AGGR_0" AS "AGGR_0" /* + materialize */,
      F0_0."GROUP_0" AS "GROUP_0",
      F0_0."GROUP_1" AS "GROUP_1",
      F0_0."GROUP_2" AS "GROUP_2",
      F1_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
    FROM
      (
        (
          SELECT
            SUM((F2_0.L_EXTENDEDPRICE * (1 - F2_0.L_DISCOUNT))) AS "AGGR_0",
            F2_0.L_ORDERKEY AS "GROUP_0",
            F1_0.O_ORDERDATE AS "GROUP_1",
            F1_0.O_SHIPPRIORITY AS "GROUP_2"
          FROM
            (
              (
                CUSTOMER AS F0_0
                CROSS JOIN ORDERS AS F1_0
              )
              CROSS JOIN LINEITEM AS F2_0
            )
          WHERE
            (
              (
                (
                  (
                    (F0_0.C_MKTSEGMENT = 'BUILDING')
                    AND (F0_0.C_CUSTKEY = F1_0.O_CUSTKEY)
                  )
                  AND (F2_0.L_ORDERKEY = F1_0.O_ORDERKEY)
                )
                AND (F1_0.O_ORDERDATE < '1995-03-15')
              )
              AND (F2_0.L_SHIPDATE > '1995-03-15')
            )
          GROUP BY
            F2_0.L_ORDERKEY,
            F1_0.O_ORDERDATE,
            F1_0.O_SHIPPRIORITY
        ) AS F0_0
        JOIN (
          SELECT
            F0_0."AGG_GB_ARG1" AS "_P_SIDE_GROUP_0",
            F0_0."AGG_GB_ARG2" AS "_P_SIDE_GROUP_1",
            F0_0."AGG_GB_ARG3" AS "_P_SIDE_GROUP_2",
            F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
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
            (
              (F0_0."GROUP_2" = F1_0."_P_SIDE_GROUP_2")
              OR (
                (F0_0."GROUP_2" IS NULL)
                AND (F1_0."_P_SIDE_GROUP_2" IS NULL)
              )
            )
            AND (
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
  ),
  temp_view_0 AS (
    SELECT
      F0_0.L_ORDERKEY AS L_ORDERKEY /* + materialize */,
      F0_0.REVENUE AS REVENUE,
      F0_0.O_ORDERDATE AS O_ORDERDATE,
      F0_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
      F1_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
      F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
    FROM
      (
        (
          SELECT
            F2_0.L_ORDERKEY AS L_ORDERKEY,
            SUM((F2_0.L_EXTENDEDPRICE * (1 - F2_0.L_DISCOUNT))) AS REVENUE,
            F1_0.O_ORDERDATE AS O_ORDERDATE,
            F1_0.O_SHIPPRIORITY AS O_SHIPPRIORITY
          FROM
            (
              (
                CUSTOMER AS F0_0
                CROSS JOIN ORDERS AS F1_0
              )
              CROSS JOIN LINEITEM AS F2_0
            )
          WHERE
            (
              (
                (
                  (
                    (F0_0.C_MKTSEGMENT = 'BUILDING')
                    AND (F0_0.C_CUSTKEY = F1_0.O_CUSTKEY)
                  )
                  AND (F2_0.L_ORDERKEY = F1_0.O_ORDERKEY)
                )
                AND (F1_0.O_ORDERDATE < '1995-03-15')
              )
              AND (F2_0.L_SHIPDATE > '1995-03-15')
            )
          GROUP BY
            F2_0.L_ORDERKEY,
            F1_0.O_ORDERDATE,
            F1_0.O_SHIPPRIORITY
          ORDER BY
            REVENUE DESC,
            O_ORDERDATE ASC
          LIMIT
            10
        ) AS F0_0
        JOIN (
          SELECT
            F0_0."GROUP_0" AS L_ORDERKEY,
            F0_0."AGGR_0" AS REVENUE,
            F0_0."GROUP_1" AS O_ORDERDATE,
            F0_0."GROUP_2" AS O_SHIPPRIORITY,
            F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
            F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
          FROM
            (
              SELECT
                *
              FROM
                temp_view_1
            ) AS F0_0
          ORDER BY
            REVENUE DESC,
            O_ORDERDATE ASC
        ) AS F1_0 ON (
          (
            (
              (
                (F0_0.L_ORDERKEY = F1_0.L_ORDERKEY)
                AND (F0_0.REVENUE = F1_0.REVENUE)
              )
              AND (F0_0.O_ORDERDATE = F1_0.O_ORDERDATE)
            )
            AND (F0_0.O_SHIPPRIORITY = F1_0.O_SHIPPRIORITY)
          )
        )
      )
  )
SELECT
  F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
  F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F0_0.L_ORDERKEY AS L_ORDERKEY,
  F0_0.O_ORDERDATE AS O_ORDERDATE,
  F0_0.O_SHIPPRIORITY AS O_SHIPPRIORITY
FROM
  (
    SELECT
      *
    FROM
      temp_view_0
  ) AS F0_0
