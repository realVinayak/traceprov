WITH
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
      F1_0.N_NATIONKEY AS N_NATIONKEY,
      F1_0.N_NAME AS N_NAME,
      F1_0.N_REGIONKEY AS N_REGIONKEY,
      F1_0.N_COMMENT AS N_COMMENT,
      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
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
            F0_0.N_NATIONKEY AS N_NATIONKEY,
            F0_0.N_NAME AS N_NAME,
            F0_0.N_REGIONKEY AS N_REGIONKEY,
            F0_0.N_COMMENT AS N_COMMENT,
            F0_0.rowid AS PROV_NATION_N__NATIONKEY,
            F0_0.rowid AS _RESULT_TID,
            1 AS _SETPROV_DUP_COUNT
          FROM
            NATION AS F0_0
        ) AS F1_0
      )
  ),
  temp_view_7 AS (
    /* + materialize */
    SELECT
      F0_1.PS_PARTKEY AS PS_PARTKEY,
      F0_1.PS_SUPPKEY AS PS_SUPPKEY,
      F0_1.PS_AVAILQTY AS PS_AVAILQTY,
      F0_1.PS_SUPPLYCOST AS PS_SUPPLYCOST,
      F0_1.PS_COMMENT AS PS_COMMENT,
      F0_1.rowid AS PROV_PARTSUPP_PS__PARTKEY,
      F0_1.rowid AS _RESULT_TID,
      1 AS _SETPROV_DUP_COUNT
    FROM
      PARTSUPP AS F0_1
  ),
  temp_view_8 AS (
    /* + materialize */
    SELECT
      F0_2.P_PARTKEY AS P_PARTKEY,
      F0_2.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
      F0_2._RESULT_TID AS _RESULT_TID,
      F0_2._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          F0_2.P_PARTKEY AS P_PARTKEY,
          F0_2.P_NAME AS P_NAME,
          F0_2.P_MFGR AS P_MFGR,
          F0_2.P_BRAND AS P_BRAND,
          F0_2.P_TYPE AS P_TYPE,
          F0_2.P_SIZE AS P_SIZE,
          F0_2.P_CONTAINER AS P_CONTAINER,
          F0_2.P_RETAILPRICE AS P_RETAILPRICE,
          F0_2.P_COMMENT AS P_COMMENT,
          F0_2.rowid AS PROV_PART_P__PARTKEY,
          F0_2.rowid AS _RESULT_TID,
          1 AS _SETPROV_DUP_COUNT
        FROM
          PART AS F0_2
      ) AS F0_2
    WHERE
      (F0_2.P_NAME LIKE 'forest%')
  ),
  temp_view_6 AS (
    /* + materialize */
    SELECT
      F0_1.PS_PARTKEY AS PS_PARTKEY,
      F0_1.PS_SUPPKEY AS PS_SUPPKEY,
      F0_1.PS_AVAILQTY AS PS_AVAILQTY,
      F0_1.PS_SUPPLYCOST AS PS_SUPPLYCOST,
      F0_1.PS_COMMENT AS PS_COMMENT,
      F0_1.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F0_1.left__RESULT_TID AS left__RESULT_TID,
      F0_1.left__SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT,
      F1_1."NESTING_EVAL_1" AS "NESTING_EVAL_1",
      F1_1.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
      F1_1.right__RESULT_TID AS right__RESULT_TID,
      F1_1.right__SETPROV_DUP_COUNT AS right__SETPROV_DUP_COUNT,
      HASH(F0_1.left__RESULT_TID, F1_1.right__RESULT_TID) AS _RESULT_TID,
      GREATEST(
        F0_1.left__SETPROV_DUP_COUNT,
        F1_1.right__SETPROV_DUP_COUNT
      ) AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          F0_1.PS_PARTKEY AS PS_PARTKEY,
          F0_1.PS_SUPPKEY AS PS_SUPPKEY,
          F0_1.PS_AVAILQTY AS PS_AVAILQTY,
          F0_1.PS_SUPPLYCOST AS PS_SUPPLYCOST,
          F0_1.PS_COMMENT AS PS_COMMENT,
          F0_1.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
          F0_1._RESULT_TID AS left__RESULT_TID,
          F0_1._SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              *
            FROM
              temp_view_7
          ) AS F0_1
      ) AS F0_1,
      LATERAL (
        SELECT
          F0_2."NESTING_EVAL_1" AS "NESTING_EVAL_1",
          F0_2.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
          F0_2._RESULT_TID AS right__RESULT_TID,
          F0_2._SETPROV_DUP_COUNT AS right__SETPROV_DUP_COUNT
        FROM
          (
            /* + materialize */
            SELECT
              (
                CASE
                  WHEN ((F0_2."NESTING_EVAL_1") IS NULL) THEN FALSE
                  WHEN (F0_2."NESTING_EVAL_1" = 1) THEN NULL
                  ELSE (F0_2."NESTING_EVAL_1" = 2)
                END
              ) AS "NESTING_EVAL_1",
              F0_2.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
              F0_2._RESULT_TID AS _RESULT_TID,
              F0_2._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
            FROM
              (
                /* + materialize */
                SELECT
                  F0_2."NESTING_EVAL_1" AS "NESTING_EVAL_1",
                  F0_2.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                  1 AS _RESULT_TID,
                  ROW_NUMBER() OVER () AS _SETPROV_DUP_COUNT
                FROM
                  (
                    SELECT
                      F0_2.NESTING_EVAL_HELP AS NESTING_EVAL_HELP,
                      F0_2.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                      F0_2._RESULT_TID AS _RESULT_TID,
                      F0_2._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT,
                      MAX(F0_2.NESTING_EVAL_HELP) OVER () AS "NESTING_EVAL_1",
                      COUNT(1) OVER () AS __DUMMY_CNT
                    FROM
                      (
                        (
                          SELECT
                            (
                              CASE
                                WHEN (F0_1.PS_PARTKEY = F0_2.P_PARTKEY) THEN 2
                                WHEN (
                                  ((F0_1.PS_PARTKEY) IS NULL)
                                  OR ((F0_2.P_PARTKEY) IS NULL)
                                ) THEN 1
                                ELSE 0
                              END
                            ) AS NESTING_EVAL_HELP,
                            F0_2.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                            F0_2._RESULT_TID AS _RESULT_TID,
                            F0_2._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
                          FROM
                            (
                              SELECT
                                *
                              FROM
                                temp_view_8
                            ) AS F0_2
                          UNION ALL
                          (
                            SELECT
                              NULL AS NESTING_EVAL_HELP,
                              NULL AS PROV_PART_P__PARTKEY,
                              -1 AS _RESULT_TID,
                              NULL AS _SETPROV_DUP_COUNT
                          )
                        )
                      ) AS F0_2
                  ) AS F0_2
                WHERE
                  (
                    (F0_2.__DUMMY_CNT = 1)
                    OR (F0_2._RESULT_TID <> -1)
                  )
              ) AS F0_2
          ) AS F0_2
      ) AS F1_1
  ),
  temp_view_5 AS (
    /* + materialize */
    SELECT
      F0_1.PS_PARTKEY AS PS_PARTKEY,
      F0_1.PS_SUPPKEY AS PS_SUPPKEY,
      F0_1.PS_AVAILQTY AS PS_AVAILQTY,
      F0_1.PS_SUPPLYCOST AS PS_SUPPLYCOST,
      F0_1.PS_COMMENT AS PS_COMMENT,
      F0_1."NESTING_EVAL_1" AS "NESTING_EVAL_1",
      F0_1.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F0_1.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
      F0_1._RESULT_TID AS _RESULT_TID,
      F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          *
        FROM
          temp_view_6
      ) AS F0_1
  ),
  temp_view_4 AS (
    /* + materialize */
    SELECT
      F0_1.PS_PARTKEY AS PS_PARTKEY,
      F0_1.PS_SUPPKEY AS PS_SUPPKEY,
      F0_1.PS_AVAILQTY AS PS_AVAILQTY,
      F0_1.PS_SUPPLYCOST AS PS_SUPPLYCOST,
      F0_1.PS_COMMENT AS PS_COMMENT,
      F0_1."NESTING_EVAL_1" AS "NESTING_EVAL_1",
      F0_1.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F0_1.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
      F0_1.left__RESULT_TID AS left__RESULT_TID,
      F0_1.left__SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT,
      F1_1."NESTING_EVAL_2" AS "NESTING_EVAL_2",
      F1_1.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F1_1.right__RESULT_TID AS right__RESULT_TID,
      F1_1.right__SETPROV_DUP_COUNT AS right__SETPROV_DUP_COUNT,
      HASH(F0_1.left__RESULT_TID, F1_1.right__RESULT_TID) AS _RESULT_TID,
      GREATEST(
        F0_1.left__SETPROV_DUP_COUNT,
        F1_1.right__SETPROV_DUP_COUNT
      ) AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          F0_1.PS_PARTKEY AS PS_PARTKEY,
          F0_1.PS_SUPPKEY AS PS_SUPPKEY,
          F0_1.PS_AVAILQTY AS PS_AVAILQTY,
          F0_1.PS_SUPPLYCOST AS PS_SUPPLYCOST,
          F0_1.PS_COMMENT AS PS_COMMENT,
          F0_1."NESTING_EVAL_1" AS "NESTING_EVAL_1",
          F0_1.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
          F0_1.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
          F0_1._RESULT_TID AS left__RESULT_TID,
          F0_1._SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              *
            FROM
              temp_view_5
          ) AS F0_1
      ) AS F0_1,
      LATERAL (
        SELECT
          F0_2."NESTING_EVAL_2" AS "NESTING_EVAL_2",
          F0_2.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
          F0_2._RESULT_TID AS right__RESULT_TID,
          F0_2._SETPROV_DUP_COUNT AS right__SETPROV_DUP_COUNT
        FROM
          (
            /* + materialize */
            SELECT
              F0_2."(0500000*SUM(L_QUANTITY))" AS "NESTING_EVAL_2",
              F0_2.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
              F0_2._RESULT_TID AS _RESULT_TID,
              F0_2._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
            FROM
              (
                /* + materialize */
                SELECT
                  (0.500000 * F0_2."AGGR_0") AS "(0500000*SUM(L_QUANTITY))",
                  F0_2.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                  F0_2._RESULT_TID AS _RESULT_TID,
                  F0_2._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
                FROM
                  (
                    /* + materialize */
                    SELECT
                      F0_2."AGGR_0" AS "AGGR_0",
                      F0_2.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                      1 AS _RESULT_TID,
                      ROW_NUMBER() OVER () AS _SETPROV_DUP_COUNT
                    FROM
                      (
                        SELECT
                          F0_2.L_ORDERKEY AS L_ORDERKEY,
                          F0_2.L_PARTKEY AS L_PARTKEY,
                          F0_2.L_SUPPKEY AS L_SUPPKEY,
                          F0_2.L_LINENUMBER AS L_LINENUMBER,
                          F0_2.L_QUANTITY AS L_QUANTITY,
                          F0_2.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                          F0_2.L_DISCOUNT AS L_DISCOUNT,
                          F0_2.L_TAX AS L_TAX,
                          F0_2.L_RETURNFLAG AS L_RETURNFLAG,
                          F0_2.L_LINESTATUS AS L_LINESTATUS,
                          F0_2.L_SHIPDATE AS L_SHIPDATE,
                          F0_2.L_COMMITDATE AS L_COMMITDATE,
                          F0_2.L_RECEIPTDATE AS L_RECEIPTDATE,
                          F0_2.L_SHIPINSTRUCT AS L_SHIPINSTRUCT,
                          F0_2.L_SHIPMODE AS L_SHIPMODE,
                          F0_2.L_COMMENT AS L_COMMENT,
                          F0_2.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                          F0_2._RESULT_TID AS _RESULT_TID,
                          F0_2._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT,
                          SUM(F0_2.L_QUANTITY) OVER () AS "AGGR_0",
                          COUNT(1) OVER () AS __DUMMY_CNT
                        FROM
                          (
                            (
                              SELECT
                                F0_2.L_ORDERKEY AS L_ORDERKEY,
                                F0_2.L_PARTKEY AS L_PARTKEY,
                                F0_2.L_SUPPKEY AS L_SUPPKEY,
                                F0_2.L_LINENUMBER AS L_LINENUMBER,
                                F0_2.L_QUANTITY AS L_QUANTITY,
                                F0_2.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                                F0_2.L_DISCOUNT AS L_DISCOUNT,
                                F0_2.L_TAX AS L_TAX,
                                F0_2.L_RETURNFLAG AS L_RETURNFLAG,
                                F0_2.L_LINESTATUS AS L_LINESTATUS,
                                F0_2.L_SHIPDATE AS L_SHIPDATE,
                                F0_2.L_COMMITDATE AS L_COMMITDATE,
                                F0_2.L_RECEIPTDATE AS L_RECEIPTDATE,
                                F0_2.L_SHIPINSTRUCT AS L_SHIPINSTRUCT,
                                F0_2.L_SHIPMODE AS L_SHIPMODE,
                                F0_2.L_COMMENT AS L_COMMENT,
                                F0_2.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                                F0_2._RESULT_TID AS _RESULT_TID,
                                F0_2._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
                              FROM
                                (
                                  SELECT
                                    F0_2.L_ORDERKEY AS L_ORDERKEY,
                                    F0_2.L_PARTKEY AS L_PARTKEY,
                                    F0_2.L_SUPPKEY AS L_SUPPKEY,
                                    F0_2.L_LINENUMBER AS L_LINENUMBER,
                                    F0_2.L_QUANTITY AS L_QUANTITY,
                                    F0_2.L_EXTENDEDPRICE AS L_EXTENDEDPRICE,
                                    F0_2.L_DISCOUNT AS L_DISCOUNT,
                                    F0_2.L_TAX AS L_TAX,
                                    F0_2.L_RETURNFLAG AS L_RETURNFLAG,
                                    F0_2.L_LINESTATUS AS L_LINESTATUS,
                                    F0_2.L_SHIPDATE AS L_SHIPDATE,
                                    F0_2.L_COMMITDATE AS L_COMMITDATE,
                                    F0_2.L_RECEIPTDATE AS L_RECEIPTDATE,
                                    F0_2.L_SHIPINSTRUCT AS L_SHIPINSTRUCT,
                                    F0_2.L_SHIPMODE AS L_SHIPMODE,
                                    F0_2.L_COMMENT AS L_COMMENT,
                                    F0_2.rowid AS PROV_LINEITEM_L__ORDERKEY,
                                    F0_2.rowid AS _RESULT_TID,
                                    1 AS _SETPROV_DUP_COUNT
                                  FROM
                                    LINEITEM AS F0_2
                                ) AS F0_2
                              WHERE
                                (
                                  (
                                    (
                                      (F0_2.L_PARTKEY = F0_1.PS_PARTKEY)
                                      AND (F0_2.L_SUPPKEY = F0_1.PS_SUPPKEY)
                                    )
                                    AND (F0_2.L_SHIPDATE >= '1994-01-01')
                                  )
                                  AND (F0_2.L_SHIPDATE < '1995-01-01')
                                )
                              UNION ALL
                              (
                                SELECT
                                  NULL AS L_ORDERKEY,
                                  NULL AS L_PARTKEY,
                                  NULL AS L_SUPPKEY,
                                  NULL AS L_LINENUMBER,
                                  NULL AS L_QUANTITY,
                                  NULL AS L_EXTENDEDPRICE,
                                  NULL AS L_DISCOUNT,
                                  NULL AS L_TAX,
                                  NULL AS L_RETURNFLAG,
                                  NULL AS L_LINESTATUS,
                                  NULL AS L_SHIPDATE,
                                  NULL AS L_COMMITDATE,
                                  NULL AS L_RECEIPTDATE,
                                  NULL AS L_SHIPINSTRUCT,
                                  NULL AS L_SHIPMODE,
                                  NULL AS L_COMMENT,
                                  NULL AS PROV_LINEITEM_L__ORDERKEY,
                                  -1 AS _RESULT_TID,
                                  NULL AS _SETPROV_DUP_COUNT
                              )
                            )
                          ) AS F0_2
                      ) AS F0_2
                    WHERE
                      (
                        (F0_2.__DUMMY_CNT = 1)
                        OR (F0_2._RESULT_TID <> -1)
                      )
                  ) AS F0_2
              ) AS F0_2
          ) AS F0_2
      ) AS F1_1
  ),
  temp_view_3 AS (
    /* + materialize */
    SELECT
      F0_1.PS_SUPPKEY AS PS_SUPPKEY,
      F0_1.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F0_1.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
      F0_1.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F0_1._RESULT_TID AS _RESULT_TID,
      F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          F0_1.PS_PARTKEY AS PS_PARTKEY,
          F0_1.PS_SUPPKEY AS PS_SUPPKEY,
          F0_1.PS_AVAILQTY AS PS_AVAILQTY,
          F0_1.PS_SUPPLYCOST AS PS_SUPPLYCOST,
          F0_1.PS_COMMENT AS PS_COMMENT,
          F0_1."NESTING_EVAL_1" AS "NESTING_EVAL_1",
          F0_1."NESTING_EVAL_2" AS "NESTING_EVAL_2",
          F0_1.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
          F0_1.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
          F0_1.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
          F0_1._RESULT_TID AS _RESULT_TID,
          F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              *
            FROM
              temp_view_4
          ) AS F0_1
      ) AS F0_1
    WHERE
      (
        F0_1."NESTING_EVAL_1"
        AND (F0_1.PS_AVAILQTY > F0_1."NESTING_EVAL_2")
      )
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
      F0_0.N_NATIONKEY AS N_NATIONKEY,
      F0_0.N_NAME AS N_NAME,
      F0_0.N_REGIONKEY AS N_REGIONKEY,
      F0_0.N_COMMENT AS N_COMMENT,
      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
      F0_0.left__RESULT_TID AS left__RESULT_TID,
      F0_0.left__SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT,
      F1_0."NESTING_EVAL_3" AS "NESTING_EVAL_3",
      F1_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F1_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
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
          F0_0.S_SUPPKEY AS S_SUPPKEY,
          F0_0.S_NAME AS S_NAME,
          F0_0.S_ADDRESS AS S_ADDRESS,
          F0_0.S_NATIONKEY AS S_NATIONKEY,
          F0_0.S_PHONE AS S_PHONE,
          F0_0.S_ACCTBAL AS S_ACCTBAL,
          F0_0.S_COMMENT AS S_COMMENT,
          F0_0.N_NATIONKEY AS N_NATIONKEY,
          F0_0.N_NAME AS N_NAME,
          F0_0.N_REGIONKEY AS N_REGIONKEY,
          F0_0.N_COMMENT AS N_COMMENT,
          F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
          F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
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
          F0_1."NESTING_EVAL_3" AS "NESTING_EVAL_3",
          F0_1.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
          F0_1.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
          F0_1.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
          F0_1._RESULT_TID AS right__RESULT_TID,
          F0_1._SETPROV_DUP_COUNT AS right__SETPROV_DUP_COUNT
        FROM
          (
            /* + materialize */
            SELECT
              (
                CASE
                  WHEN ((F0_1."NESTING_EVAL_3") IS NULL) THEN FALSE
                  WHEN (F0_1."NESTING_EVAL_3" = 1) THEN NULL
                  ELSE (F0_1."NESTING_EVAL_3" = 2)
                END
              ) AS "NESTING_EVAL_3",
              F0_1.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
              F0_1.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
              F0_1.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
              F0_1._RESULT_TID AS _RESULT_TID,
              F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
            FROM
              (
                /* + materialize */
                SELECT
                  F0_1."NESTING_EVAL_3" AS "NESTING_EVAL_3",
                  F0_1.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                  F0_1.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                  F0_1.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                  1 AS _RESULT_TID,
                  ROW_NUMBER() OVER () AS _SETPROV_DUP_COUNT
                FROM
                  (
                    SELECT
                      F0_1.NESTING_EVAL_HELP AS NESTING_EVAL_HELP,
                      F0_1.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                      F0_1.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                      F0_1.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                      F0_1._RESULT_TID AS _RESULT_TID,
                      F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT,
                      MAX(
                        (
                          CASE
                            WHEN (1 = F0_1._SETPROV_DUP_COUNT) THEN F0_1.NESTING_EVAL_HELP
                            ELSE NULL
                          END
                        )
                      ) OVER () AS "NESTING_EVAL_3",
                      COUNT(1) OVER () AS __DUMMY_CNT
                    FROM
                      (
                        (
                          SELECT
                            (
                              CASE
                                WHEN (F0_0.S_SUPPKEY = F0_1.PS_SUPPKEY) THEN 2
                                WHEN (
                                  ((F0_0.S_SUPPKEY) IS NULL)
                                  OR ((F0_1.PS_SUPPKEY) IS NULL)
                                ) THEN 1
                                ELSE 0
                              END
                            ) AS NESTING_EVAL_HELP,
                            F0_1.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                            F0_1.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                            F0_1.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                            F0_1._RESULT_TID AS _RESULT_TID,
                            F0_1._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
                          FROM
                            (
                              SELECT
                                *
                              FROM
                                temp_view_3
                            ) AS F0_1
                          UNION ALL
                          (
                            SELECT
                              NULL AS NESTING_EVAL_HELP,
                              NULL AS PROV_PARTSUPP_PS__PARTKEY,
                              NULL AS PROV_PART_P__PARTKEY,
                              NULL AS PROV_LINEITEM_L__ORDERKEY,
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
              ) AS F0_1
          ) AS F0_1
      ) AS F1_0
  ),
  temp_view_0 AS (
    /* + materialize */
    SELECT
      F0_0.S_NAME AS S_NAME,
      F0_0.S_ADDRESS AS S_ADDRESS,
      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
      F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
    FROM
      (
        SELECT
          F0_0.S_NAME AS S_NAME,
          F0_0.S_ADDRESS AS S_ADDRESS,
          F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
          F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
          F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
          F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
          F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
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
              F0_0.N_NATIONKEY AS N_NATIONKEY,
              F0_0.N_NAME AS N_NAME,
              F0_0.N_REGIONKEY AS N_REGIONKEY,
              F0_0.N_COMMENT AS N_COMMENT,
              F0_0."NESTING_EVAL_3" AS "NESTING_EVAL_3",
              F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
              F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
              F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
              F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
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
          ) AS F0_0
        WHERE
          (
            (
              F0_0."NESTING_EVAL_3"
              AND (F0_0.S_NATIONKEY = F0_0.N_NATIONKEY)
            )
            AND (F0_0.N_NAME = 'CANADA')
          )
        ORDER BY
          S_NAME ASC
      ) AS F0_0
  )
SELECT
  F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
  F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
  F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
  F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
FROM
  (
    SELECT
      *
    FROM
      temp_view_0
  ) AS F0_0
