WITH
  temp_view_5 AS (
    SELECT
      F0_0."AGGR_0" AS "AGGR_0" /* + materialize */,
      F0_0."GROUP_0" AS "GROUP_0",
      F0_0."GROUP_1" AS "GROUP_1",
      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
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
            SUM(F0_0.L_QUANTITY) AS "AGGR_0",
            F0_0.L_PARTKEY AS "GROUP_0",
            F0_0.L_SUPPKEY AS "GROUP_1"
          FROM
            LINEITEM AS F0_0
          WHERE
            (
              (F0_0.L_SHIPDATE >= '1994-01-01')
              AND (F0_0.L_SHIPDATE < '1995-01-01')
            )
          GROUP BY
            F0_0.L_PARTKEY,
            F0_0.L_SUPPKEY
        ) AS F0_0
        JOIN (
          SELECT
            F0_0.L_PARTKEY AS "_P_SIDE_GROUP_0",
            F0_0.L_SUPPKEY AS "_P_SIDE_GROUP_1",
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
                F0_0.rowid AS PROV_LINEITEM_L__ORDERKEY,
                F0_0.rowid AS _RESULT_TID,
                1 AS _SETPROV_DUP_COUNT
              FROM
                LINEITEM AS F0_0
            ) AS F0_0
          WHERE
            (
              (F0_0.L_SHIPDATE >= '1994-01-01')
              AND (F0_0.L_SHIPDATE < '1995-01-01')
            )
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
  ),
  temp_view_4 AS (
    SELECT
      F0_0.PS_PARTKEY AS PS_PARTKEY /* + materialize */,
      F0_0.PS_SUPPKEY AS PS_SUPPKEY,
      F0_0.PS_AVAILQTY AS PS_AVAILQTY,
      F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
      F0_0.PS_COMMENT AS PS_COMMENT,
      F0_0.P_PARTKEY AS P_PARTKEY,
      F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
      F0_0._RESULT_TID AS left__RESULT_TID,
      F0_0._SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT,
      F1_0.COMPUTED AS COMPUTED,
      F1_0.L_PARTKEY AS L_PARTKEY,
      F1_0.L_SUPPKEY AS L_SUPPKEY,
      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F1_0._RESULT_TID AS right__RESULT_TID,
      F1_0._SETPROV_DUP_COUNT AS right__SETPROV_DUP_COUNT,
      HASH(F0_0._RESULT_TID, F1_0._RESULT_TID) AS _RESULT_TID,
      GREATEST(F0_0._SETPROV_DUP_COUNT, F1_0._SETPROV_DUP_COUNT) AS _SETPROV_DUP_COUNT
    FROM
      (
        (
          SELECT
            F0_0.PS_PARTKEY AS PS_PARTKEY,
            F0_0.PS_SUPPKEY AS PS_SUPPKEY,
            F0_0.PS_AVAILQTY AS PS_AVAILQTY,
            F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
            F0_0.PS_COMMENT AS PS_COMMENT,
            F1_0.P_PARTKEY AS P_PARTKEY,
            F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
            F1_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
            HASH(F0_0._RESULT_TID, F1_0._RESULT_TID) AS _RESULT_TID,
            GREATEST(F0_0._SETPROV_DUP_COUNT, F1_0._SETPROV_DUP_COUNT) AS _SETPROV_DUP_COUNT
          FROM
            (
              (
                SELECT
                  F0_0.PS_PARTKEY AS PS_PARTKEY,
                  F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                  F0_0.PS_AVAILQTY AS PS_AVAILQTY,
                  F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
                  F0_0.PS_COMMENT AS PS_COMMENT,
                  F0_0.rowid AS PROV_PARTSUPP_PS__PARTKEY,
                  F0_0.rowid AS _RESULT_TID,
                  1 AS _SETPROV_DUP_COUNT
                FROM
                  PARTSUPP AS F0_0
              ) AS F0_0
              JOIN (
                SELECT
                  F0_0.P_PARTKEY AS P_PARTKEY,
                  F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
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
                      F0_0.rowid AS PROV_PART_P__PARTKEY,
                      F0_0.rowid AS _RESULT_TID,
                      1 AS _SETPROV_DUP_COUNT
                    FROM
                      PART AS F0_0
                  ) AS F0_0
                WHERE
                  (F0_0.P_NAME LIKE 'forest%')
              ) AS F1_0 ON ((F1_0.P_PARTKEY = F0_0.PS_PARTKEY))
            )
        ) AS F0_0
        JOIN (
          SELECT
            (0.500000 * F0_0."AGGR_0") AS COMPUTED,
            F0_0."GROUP_0" AS L_PARTKEY,
            F0_0."GROUP_1" AS L_SUPPKEY,
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
        ) AS F1_0 ON (
          (
            (
              (F0_0.PS_PARTKEY = F1_0.L_PARTKEY)
              AND (F0_0.PS_SUPPKEY = F1_0.L_SUPPKEY)
            )
            AND (F0_0.PS_AVAILQTY > F1_0.COMPUTED)
          )
        )
      )
  ),
  temp_view_3 AS (
    SELECT
      F0_0.PS_PARTKEY AS PS_PARTKEY /* + materialize */,
      F0_0.PS_SUPPKEY AS PS_SUPPKEY,
      F0_0.PS_AVAILQTY AS PS_AVAILQTY,
      F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
      F0_0.PS_COMMENT AS PS_COMMENT,
      F0_0.P_PARTKEY AS P_PARTKEY,
      F0_0.COMPUTED AS COMPUTED,
      F0_0.L_PARTKEY AS L_PARTKEY,
      F0_0.L_SUPPKEY AS L_SUPPKEY,
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
          temp_view_4
      ) AS F0_0
  ),
  temp_view_2 AS (
    SELECT
      F0_0."GROUP_0" AS "GROUP_0" /* + materialize */,
      F1_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F1_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
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
            F0_0.PS_SUPPKEY AS "GROUP_0"
          FROM
            (
              (
                PARTSUPP AS F0_0
                JOIN (
                  SELECT
                    F0_0.P_PARTKEY AS P_PARTKEY
                  FROM
                    PART AS F0_0
                  WHERE
                    (F0_0.P_NAME LIKE 'forest%')
                ) AS F1_0 ON ((F1_0.P_PARTKEY = F0_0.PS_PARTKEY))
              )
              JOIN (
                SELECT
                  (0.500000 * SUM(F0_0.L_QUANTITY)) AS COMPUTED,
                  F0_0.L_PARTKEY AS L_PARTKEY,
                  F0_0.L_SUPPKEY AS L_SUPPKEY
                FROM
                  LINEITEM AS F0_0
                WHERE
                  (
                    (F0_0.L_SHIPDATE >= '1994-01-01')
                    AND (F0_0.L_SHIPDATE < '1995-01-01')
                  )
                GROUP BY
                  F0_0.L_PARTKEY,
                  F0_0.L_SUPPKEY
              ) AS F2_0 ON (
                (
                  (
                    (F0_0.PS_PARTKEY = F2_0.L_PARTKEY)
                    AND (F0_0.PS_SUPPKEY = F2_0.L_SUPPKEY)
                  )
                  AND (F0_0.PS_AVAILQTY > F2_0.COMPUTED)
                )
              )
            )
          GROUP BY
            F0_0.PS_SUPPKEY
        ) AS F0_0
        JOIN (
          SELECT
            F0_0.PS_SUPPKEY AS "_P_SIDE_GROUP_0",
            F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
            F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
          FROM
            (
              SELECT
                *
              FROM
                temp_view_3
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
  temp_view_1 AS (
    SELECT
      F0_0.S_SUPPKEY AS S_SUPPKEY /* + materialize */,
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
      F0_0._SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT,
      F1_0.PS_SUPPKEY AS PS_SUPPKEY,
      F1_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F1_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
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
        ) AS F0_0
        CROSS JOIN (
          SELECT
            F0_0."GROUP_0" AS PS_SUPPKEY,
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
                temp_view_2
            ) AS F0_0
        ) AS F1_0
      )
  ),
  temp_view_0 AS (
    SELECT
      F0_0.S_NAME AS S_NAME /* + materialize */,
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
              F0_0.PS_SUPPKEY AS PS_SUPPKEY,
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
              (F0_0.PS_SUPPKEY = F0_0.S_SUPPKEY)
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
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F0_0.S_NAME AS S_NAME,
  F0_0.S_ADDRESS AS S_ADDRESS
FROM
  (
    SELECT
      *
    FROM
      temp_view_0
  ) AS F0_0
