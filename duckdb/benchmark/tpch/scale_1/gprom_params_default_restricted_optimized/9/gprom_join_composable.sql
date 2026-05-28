WITH
  temp_view_2 AS (
    SELECT
      /*+ materialize */ F0_0.N_NAME AS NATION,
      DATE_PART('YEAR', (F0_0.O_ORDERDATE)::DATE) AS O_YEAR,
      (
        (F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT)) - (F0_0.PS_SUPPLYCOST * F0_0.L_QUANTITY)
      ) AS AMOUNT,
      F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
      F0_0._RESULT_TID AS _RESULT_TID,
      F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          F0_0.P_PARTKEY AS P_PARTKEY,
          F0_0.P_NAME AS P_NAME,
          F0_0.P_MFGR AS P_MFGR,
          F0_0.P_BRAND AS P_BRAND,
          F0_0.P_TYPE AS P_TYPE,
          F0_0.P_SIZE AS P_SIZE,
          F0_0.P_CONTAINER AS P_CONTAINER,
          F0_0.P_RETAILPRICE AS P_RETAILPRICE,
          F0_0.P_COMMENT AS P_COMMENT,
          F0_0.S_SUPPKEY AS S_SUPPKEY,
          F0_0.S_NAME AS S_NAME,
          F0_0.S_ADDRESS AS S_ADDRESS,
          F0_0.S_NATIONKEY AS S_NATIONKEY,
          F0_0.S_PHONE AS S_PHONE,
          F0_0.S_ACCTBAL AS S_ACCTBAL,
          F0_0.S_COMMENT AS S_COMMENT,
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
          F0_0.PS_PARTKEY AS PS_PARTKEY,
          F0_0.PS_SUPPKEY AS PS_SUPPKEY,
          F0_0.PS_AVAILQTY AS PS_AVAILQTY,
          F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
          F0_0.PS_COMMENT AS PS_COMMENT,
          F0_0.O_ORDERKEY AS O_ORDERKEY,
          F0_0.O_CUSTKEY AS O_CUSTKEY,
          F0_0.O_ORDERSTATUS AS O_ORDERSTATUS,
          F0_0.O_TOTALPRICE AS O_TOTALPRICE,
          F0_0.O_ORDERDATE AS O_ORDERDATE,
          F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
          F0_0.O_CLERK AS O_CLERK,
          F0_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
          F0_0.O_COMMENT AS O_COMMENT,
          F1_0.N_NATIONKEY AS N_NATIONKEY,
          F1_0.N_NAME AS N_NAME,
          F1_0.N_REGIONKEY AS N_REGIONKEY,
          F1_0.N_COMMENT AS N_COMMENT,
          F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
          F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
          F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
          F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
          F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
          F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
          hash(F0_0._RESULT_TID, F1_0._RESULT_TID) AS _RESULT_TID,
          GREATEST(F0_0._SETPROV_DUP_COUNT, F1_0._SETPROV_DUP_COUNT) AS _SETPROV_DUP_COUNT
        FROM
          (
            (
              SELECT
                F0_0.P_PARTKEY AS P_PARTKEY,
                F0_0.P_NAME AS P_NAME,
                F0_0.P_MFGR AS P_MFGR,
                F0_0.P_BRAND AS P_BRAND,
                F0_0.P_TYPE AS P_TYPE,
                F0_0.P_SIZE AS P_SIZE,
                F0_0.P_CONTAINER AS P_CONTAINER,
                F0_0.P_RETAILPRICE AS P_RETAILPRICE,
                F0_0.P_COMMENT AS P_COMMENT,
                F0_0.S_SUPPKEY AS S_SUPPKEY,
                F0_0.S_NAME AS S_NAME,
                F0_0.S_ADDRESS AS S_ADDRESS,
                F0_0.S_NATIONKEY AS S_NATIONKEY,
                F0_0.S_PHONE AS S_PHONE,
                F0_0.S_ACCTBAL AS S_ACCTBAL,
                F0_0.S_COMMENT AS S_COMMENT,
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
                F0_0.PS_PARTKEY AS PS_PARTKEY,
                F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                F0_0.PS_AVAILQTY AS PS_AVAILQTY,
                F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
                F0_0.PS_COMMENT AS PS_COMMENT,
                F1_0.O_ORDERKEY AS O_ORDERKEY,
                F1_0.O_CUSTKEY AS O_CUSTKEY,
                F1_0.O_ORDERSTATUS AS O_ORDERSTATUS,
                F1_0.O_TOTALPRICE AS O_TOTALPRICE,
                F1_0.O_ORDERDATE AS O_ORDERDATE,
                F1_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
                F1_0.O_CLERK AS O_CLERK,
                F1_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
                F1_0.O_COMMENT AS O_COMMENT,
                F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                hash(F0_0._RESULT_TID, F1_0._RESULT_TID) AS _RESULT_TID,
                GREATEST(F0_0._SETPROV_DUP_COUNT, F1_0._SETPROV_DUP_COUNT) AS _SETPROV_DUP_COUNT
              FROM
                (
                  (
                    SELECT
                      F0_0.P_PARTKEY AS P_PARTKEY,
                      F0_0.P_NAME AS P_NAME,
                      F0_0.P_MFGR AS P_MFGR,
                      F0_0.P_BRAND AS P_BRAND,
                      F0_0.P_TYPE AS P_TYPE,
                      F0_0.P_SIZE AS P_SIZE,
                      F0_0.P_CONTAINER AS P_CONTAINER,
                      F0_0.P_RETAILPRICE AS P_RETAILPRICE,
                      F0_0.P_COMMENT AS P_COMMENT,
                      F0_0.S_SUPPKEY AS S_SUPPKEY,
                      F0_0.S_NAME AS S_NAME,
                      F0_0.S_ADDRESS AS S_ADDRESS,
                      F0_0.S_NATIONKEY AS S_NATIONKEY,
                      F0_0.S_PHONE AS S_PHONE,
                      F0_0.S_ACCTBAL AS S_ACCTBAL,
                      F0_0.S_COMMENT AS S_COMMENT,
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
                      F1_0.PS_PARTKEY AS PS_PARTKEY,
                      F1_0.PS_SUPPKEY AS PS_SUPPKEY,
                      F1_0.PS_AVAILQTY AS PS_AVAILQTY,
                      F1_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
                      F1_0.PS_COMMENT AS PS_COMMENT,
                      F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                      F1_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                      hash(F0_0._RESULT_TID, F1_0._RESULT_TID) AS _RESULT_TID,
                      GREATEST(F0_0._SETPROV_DUP_COUNT, F1_0._SETPROV_DUP_COUNT) AS _SETPROV_DUP_COUNT
                    FROM
                      (
                        (
                          SELECT
                            F0_0.P_PARTKEY AS P_PARTKEY,
                            F0_0.P_NAME AS P_NAME,
                            F0_0.P_MFGR AS P_MFGR,
                            F0_0.P_BRAND AS P_BRAND,
                            F0_0.P_TYPE AS P_TYPE,
                            F0_0.P_SIZE AS P_SIZE,
                            F0_0.P_CONTAINER AS P_CONTAINER,
                            F0_0.P_RETAILPRICE AS P_RETAILPRICE,
                            F0_0.P_COMMENT AS P_COMMENT,
                            F0_0.S_SUPPKEY AS S_SUPPKEY,
                            F0_0.S_NAME AS S_NAME,
                            F0_0.S_ADDRESS AS S_ADDRESS,
                            F0_0.S_NATIONKEY AS S_NATIONKEY,
                            F0_0.S_PHONE AS S_PHONE,
                            F0_0.S_ACCTBAL AS S_ACCTBAL,
                            F0_0.S_COMMENT AS S_COMMENT,
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
                            F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                            F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                            F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                            hash(F0_0._RESULT_TID, F1_0._RESULT_TID) AS _RESULT_TID,
                            GREATEST(F0_0._SETPROV_DUP_COUNT, F1_0._SETPROV_DUP_COUNT) AS _SETPROV_DUP_COUNT
                          FROM
                            (
                              (
                                SELECT
                                  F0_0.P_PARTKEY AS P_PARTKEY,
                                  F0_0.P_NAME AS P_NAME,
                                  F0_0.P_MFGR AS P_MFGR,
                                  F0_0.P_BRAND AS P_BRAND,
                                  F0_0.P_TYPE AS P_TYPE,
                                  F0_0.P_SIZE AS P_SIZE,
                                  F0_0.P_CONTAINER AS P_CONTAINER,
                                  F0_0.P_RETAILPRICE AS P_RETAILPRICE,
                                  F0_0.P_COMMENT AS P_COMMENT,
                                  F1_0.S_SUPPKEY AS S_SUPPKEY,
                                  F1_0.S_NAME AS S_NAME,
                                  F1_0.S_ADDRESS AS S_ADDRESS,
                                  F1_0.S_NATIONKEY AS S_NATIONKEY,
                                  F1_0.S_PHONE AS S_PHONE,
                                  F1_0.S_ACCTBAL AS S_ACCTBAL,
                                  F1_0.S_COMMENT AS S_COMMENT,
                                  F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                                  F1_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                                  hash(F0_0._RESULT_TID, F1_0._RESULT_TID) AS _RESULT_TID,
                                  GREATEST(F0_0._SETPROV_DUP_COUNT, F1_0._SETPROV_DUP_COUNT) AS _SETPROV_DUP_COUNT
                                FROM
                                  (
                                    (
                                      SELECT
                                        F0_0.P_PARTKEY AS P_PARTKEY,
                                        F0_0.P_NAME AS P_NAME,
                                        F0_0.P_MFGR AS P_MFGR,
                                        F0_0.P_BRAND AS P_BRAND,
                                        F0_0.P_TYPE AS P_TYPE,
                                        F0_0.P_SIZE AS P_SIZE,
                                        F0_0.P_CONTAINER AS P_CONTAINER,
                                        F0_0.P_RETAILPRICE AS P_RETAILPRICE,
                                        F0_0.P_COMMENT AS P_COMMENT,
                                        F0_0.P_PARTKEY AS PROV_PART_P__PARTKEY,
                                        F0_0.rowid AS _RESULT_TID,
                                        1 AS _SETPROV_DUP_COUNT
                                      FROM
                                        PART F0_0
                                    ) F0_0
                                    CROSS JOIN (
                                      SELECT
                                        F0_0.S_SUPPKEY AS S_SUPPKEY,
                                        F0_0.S_NAME AS S_NAME,
                                        F0_0.S_ADDRESS AS S_ADDRESS,
                                        F0_0.S_NATIONKEY AS S_NATIONKEY,
                                        F0_0.S_PHONE AS S_PHONE,
                                        F0_0.S_ACCTBAL AS S_ACCTBAL,
                                        F0_0.S_COMMENT AS S_COMMENT,
                                        F0_0.S_SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                                        F0_0.rowid AS _RESULT_TID,
                                        1 AS _SETPROV_DUP_COUNT
                                      FROM
                                        SUPPLIER F0_0
                                    ) F1_0
                                  )
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
                        CROSS JOIN (
                          SELECT
                            F0_0.PS_PARTKEY AS PS_PARTKEY,
                            F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                            F0_0.PS_AVAILQTY AS PS_AVAILQTY,
                            F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
                            F0_0.PS_COMMENT AS PS_COMMENT,
                            F0_0.PS_PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                            F0_0.rowid AS _RESULT_TID,
                            1 AS _SETPROV_DUP_COUNT
                          FROM
                            PARTSUPP F0_0
                        ) F1_0
                      )
                  ) F0_0
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
                      F0_0.O_ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                      F0_0.rowid AS _RESULT_TID,
                      1 AS _SETPROV_DUP_COUNT
                    FROM
                      ORDERS F0_0
                  ) F1_0
                )
            ) F0_0
            CROSS JOIN (
              SELECT
                F0_0.N_NATIONKEY AS N_NATIONKEY,
                F0_0.N_NAME AS N_NAME,
                F0_0.N_REGIONKEY AS N_REGIONKEY,
                F0_0.N_COMMENT AS N_COMMENT,
                F0_0.N_NATIONKEY AS PROV_NATION_N__NATIONKEY,
                F0_0.rowid AS _RESULT_TID,
                1 AS _SETPROV_DUP_COUNT
              FROM
                NATION F0_0
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
                  (F0_0.S_SUPPKEY = F0_0.L_SUPPKEY)
                  AND (F0_0.PS_SUPPKEY = F0_0.L_SUPPKEY)
                )
                AND (F0_0.PS_PARTKEY = F0_0.L_PARTKEY)
              )
              AND (F0_0.P_PARTKEY = F0_0.L_PARTKEY)
            )
            AND (F0_0.O_ORDERKEY = F0_0.L_ORDERKEY)
          )
          AND (F0_0.S_NATIONKEY = F0_0.N_NATIONKEY)
        )
        AND (F0_0.P_NAME LIKE '%green%')
      )
  ),
  temp_view_1 AS (
    SELECT
      /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0",
      F0_0."GROUP_0" AS "GROUP_0",
      F0_0."GROUP_1" AS "GROUP_1",
      F1_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
      F1_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F1_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
      DENSE_RANK () OVER (
        ORDER BY
          F0_0."GROUP_0",
          F0_0."GROUP_1"
      ) AS _RESULT_TID,
      ROW_NUMBER() OVER (
        PARTITION BY
          F0_0."GROUP_0",
          F0_0."GROUP_1"
        ORDER BY
          F0_0."GROUP_0",
          F0_0."GROUP_1"
      ) AS _SETPROV_DUP_COUNT
    FROM
      (
        (
          SELECT
            SUM(
              (
                (F2_0.L_EXTENDEDPRICE * (1 - F2_0.L_DISCOUNT)) - (F3_0.PS_SUPPLYCOST * F2_0.L_QUANTITY)
              )
            ) AS "AGGR_0",
            F5_0.N_NAME AS "GROUP_0",
            DATE_PART('YEAR', (F4_0.O_ORDERDATE)::DATE) AS "GROUP_1"
          FROM
            (
              (
                (
                  (
                    (
                      PART F0_0
                      CROSS JOIN SUPPLIER F1_0
                    )
                    CROSS JOIN LINEITEM F2_0
                  )
                  CROSS JOIN PARTSUPP F3_0
                )
                CROSS JOIN ORDERS F4_0
              )
              CROSS JOIN NATION F5_0
            )
          WHERE
            (
              (
                (
                  (
                    (
                      (
                        (F1_0.S_SUPPKEY = F2_0.L_SUPPKEY)
                        AND (F3_0.PS_SUPPKEY = F2_0.L_SUPPKEY)
                      )
                      AND (F3_0.PS_PARTKEY = F2_0.L_PARTKEY)
                    )
                    AND (F0_0.P_PARTKEY = F2_0.L_PARTKEY)
                  )
                  AND (F4_0.O_ORDERKEY = F2_0.L_ORDERKEY)
                )
                AND (F1_0.S_NATIONKEY = F5_0.N_NATIONKEY)
              )
              AND (F0_0.P_NAME LIKE '%green%')
            )
          GROUP BY
            F5_0.N_NAME,
            DATE_PART('YEAR', (F4_0.O_ORDERDATE)::DATE)
        ) F0_0
        JOIN (
          SELECT
            F0_0.NATION AS "_P_SIDE_GROUP_0",
            F0_0.O_YEAR AS "_P_SIDE_GROUP_1",
            F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
            F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
            F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
            F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
            F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY
          FROM
            (
              SELECT
                *
              FROM
                temp_view_2
            ) F0_0
        ) F1_0 ON (
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
  ),
  temp_view_0 AS (
    SELECT
      /*+ materialize */ F0_0.NATION AS NATION,
      F0_0.O_YEAR AS O_YEAR,
      F0_0.SUM_PROFIT AS SUM_PROFIT,
      F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
      F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY
    FROM
      (
        SELECT
          F0_0."GROUP_0" AS NATION,
          F0_0."GROUP_1" AS O_YEAR,
          F0_0."AGGR_0" AS SUM_PROFIT,
          F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
          F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
          F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
          F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
          F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
          F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
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
          NATION ASC NULLS LAST,
          O_YEAR DESC NULLS LAST
      ) F0_0
  )
SELECT
  F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
  F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
  F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
  F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY
FROM
  (
    SELECT
      *
    FROM
      temp_view_0
  ) F0_0;
