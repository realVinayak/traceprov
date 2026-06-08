WITH
  temp_view_3 AS (
    SELECT
      (F0_0.PS_SUPPLYCOST * F0_0.PS_AVAILQTY) AS "AGG_GB_ARG0" /* + materialize */,
      F0_0.PS_PARTKEY AS "AGG_GB_ARG1",
      F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
      F0_0._RESULT_TID AS _RESULT_TID,
      F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
    FROM
      (
        SELECT
          F0_0.PS_PARTKEY AS PS_PARTKEY,
          F0_0.PS_SUPPKEY AS PS_SUPPKEY,
          F0_0.PS_AVAILQTY AS PS_AVAILQTY,
          F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
          F0_0.PS_COMMENT AS PS_COMMENT,
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
          F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
          F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
          F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
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
                F1_0.S_SUPPKEY AS S_SUPPKEY,
                F1_0.S_NAME AS S_NAME,
                F1_0.S_ADDRESS AS S_ADDRESS,
                F1_0.S_NATIONKEY AS S_NATIONKEY,
                F1_0.S_PHONE AS S_PHONE,
                F1_0.S_ACCTBAL AS S_ACCTBAL,
                F1_0.S_COMMENT AS S_COMMENT,
                F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
                F1_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
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
                  CROSS JOIN (
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
                  ) AS F1_0
                )
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
    WHERE
      (
        (
          (F0_0.PS_SUPPKEY = F0_0.S_SUPPKEY)
          AND (F0_0.S_NATIONKEY = F0_0.N_NATIONKEY)
        )
        AND (F0_0.N_NAME = 'GERMANY')
      )
  ),
  temp_view_2 AS (
    SELECT
      F0_0."AGGR_0" AS "AGGR_0" /* + materialize */,
      F0_0."GROUP_0" AS "GROUP_0",
      F1_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F1_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
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
            SUM((F0_0.PS_SUPPLYCOST * F0_0.PS_AVAILQTY)) AS "AGGR_0",
            F0_0.PS_PARTKEY AS "GROUP_0"
          FROM
            (
              (
                PARTSUPP AS F0_0
                CROSS JOIN SUPPLIER AS F1_0
              )
              CROSS JOIN NATION AS F2_0
            )
          WHERE
            (
              (
                (F0_0.PS_SUPPKEY = F1_0.S_SUPPKEY)
                AND (F1_0.S_NATIONKEY = F2_0.N_NATIONKEY)
              )
              AND (F2_0.N_NAME = 'GERMANY')
            )
          GROUP BY
            F0_0.PS_PARTKEY
        ) AS F0_0
        JOIN (
          SELECT
            F0_0."AGG_GB_ARG1" AS "_P_SIDE_GROUP_0",
            F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
            F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
            F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY
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
  temp_view_4 AS (
    SELECT
      F0_0."AGGR_0" AS "AGGR_0" /* + materialize */,
      F1_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
      F1_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
      F1_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
      1 AS _RESULT_TID,
      ROW_NUMBER() OVER () AS _SETPROV_DUP_COUNT
    FROM
      (
        (
          SELECT
            SUM((F0_0.PS_SUPPLYCOST * F0_0.PS_AVAILQTY)) AS "AGGR_0"
          FROM
            (
              (
                PARTSUPP AS F0_0
                CROSS JOIN SUPPLIER AS F1_0
              )
              CROSS JOIN NATION AS F2_0
            )
          WHERE
            (
              (
                (F0_0.PS_SUPPKEY = F1_0.S_SUPPKEY)
                AND (F1_0.S_NATIONKEY = F2_0.N_NATIONKEY)
              )
              AND (F2_0.N_NAME = 'GERMANY')
            )
        ) AS F0_0
        LEFT OUTER JOIN (
          SELECT
            (F0_0.PS_SUPPLYCOST * F0_0.PS_AVAILQTY) AS "AGG_GB_ARG0",
            F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
            F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
            F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
            F0_0._RESULT_TID AS _RESULT_TID,
            F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
          FROM
            (
              SELECT
                F0_0.PS_PARTKEY AS PS_PARTKEY,
                F0_0.PS_SUPPKEY AS PS_SUPPKEY,
                F0_0.PS_AVAILQTY AS PS_AVAILQTY,
                F0_0.PS_SUPPLYCOST AS PS_SUPPLYCOST,
                F0_0.PS_COMMENT AS PS_COMMENT,
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
                F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
                F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
                F1_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
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
                      F1_0.S_SUPPKEY AS S_SUPPKEY,
                      F1_0.S_NAME AS S_NAME,
                      F1_0.S_ADDRESS AS S_ADDRESS,
                      F1_0.S_NATIONKEY AS S_NATIONKEY,
                      F1_0.S_PHONE AS S_PHONE,
                      F1_0.S_ACCTBAL AS S_ACCTBAL,
                      F1_0.S_COMMENT AS S_COMMENT,
                      F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
                      F1_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
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
                            F0_0.rowid AS PROV_PARTSUPP_1_PS__PARTKEY,
                            F0_0.rowid AS _RESULT_TID,
                            1 AS _SETPROV_DUP_COUNT
                          FROM
                            PARTSUPP AS F0_0
                        ) AS F0_0
                        CROSS JOIN (
                          SELECT
                            F0_0.S_SUPPKEY AS S_SUPPKEY,
                            F0_0.S_NAME AS S_NAME,
                            F0_0.S_ADDRESS AS S_ADDRESS,
                            F0_0.S_NATIONKEY AS S_NATIONKEY,
                            F0_0.S_PHONE AS S_PHONE,
                            F0_0.S_ACCTBAL AS S_ACCTBAL,
                            F0_0.S_COMMENT AS S_COMMENT,
                            F0_0.rowid AS PROV_SUPPLIER_1_S__SUPPKEY,
                            F0_0.rowid AS _RESULT_TID,
                            1 AS _SETPROV_DUP_COUNT
                          FROM
                            SUPPLIER AS F0_0
                        ) AS F1_0
                      )
                  ) AS F0_0
                  CROSS JOIN (
                    SELECT
                      F0_0.N_NATIONKEY AS N_NATIONKEY,
                      F0_0.N_NAME AS N_NAME,
                      F0_0.N_REGIONKEY AS N_REGIONKEY,
                      F0_0.N_COMMENT AS N_COMMENT,
                      F0_0.rowid AS PROV_NATION_1_N__NATIONKEY,
                      F0_0.rowid AS _RESULT_TID,
                      1 AS _SETPROV_DUP_COUNT
                    FROM
                      NATION AS F0_0
                  ) AS F1_0
                )
            ) AS F0_0
          WHERE
            (
              (
                (F0_0.PS_SUPPKEY = F0_0.S_SUPPKEY)
                AND (F0_0.S_NATIONKEY = F0_0.N_NATIONKEY)
              )
              AND (F0_0.N_NAME = 'GERMANY')
            )
        ) AS F1_0 ON ((1 = 1))
      )
  ),
  temp_view_1 AS (
    SELECT
      F0_0.PS_PARTKEY AS PS_PARTKEY /* + materialize */,
      F0_0.VALUE AS VALUE,
      F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
      F0_0._RESULT_TID AS left__RESULT_TID,
      F0_0._SETPROV_DUP_COUNT AS left__SETPROV_DUP_COUNT,
      F1_0."(SUM((PS_SUPPLYCOST*PS_AVAILQTY))*0000010)" AS "(SUM((PS_SUPPLYCOST*PS_AVAILQTY))*0000010)",
      F1_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
      F1_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
      F1_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
      F1_0._RESULT_TID AS right__RESULT_TID,
      F1_0._SETPROV_DUP_COUNT AS right__SETPROV_DUP_COUNT,
      HASH(F0_0._RESULT_TID, F1_0._RESULT_TID) AS _RESULT_TID,
      GREATEST(F0_0._SETPROV_DUP_COUNT, F1_0._SETPROV_DUP_COUNT) AS _SETPROV_DUP_COUNT
    FROM
      (
        (
          SELECT
            F0_0."GROUP_0" AS PS_PARTKEY,
            F0_0."AGGR_0" AS VALUE,
            F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
            F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
            F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
            F0_0._RESULT_TID AS _RESULT_TID,
            F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
          FROM
            (
              SELECT
                *
              FROM
                temp_view_2
            ) AS F0_0
        ) AS F0_0
        CROSS JOIN (
          SELECT
            (F0_0."AGGR_0" * 0.000010) AS "(SUM((PS_SUPPLYCOST*PS_AVAILQTY))*0000010)",
            F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
            F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
            F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
            F0_0._RESULT_TID AS _RESULT_TID,
            F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
          FROM
            (
              SELECT
                *
              FROM
                temp_view_4
            ) AS F0_0
        ) AS F1_0
      )
  ),
  temp_view_0 AS (
    SELECT
      F0_0.PS_PARTKEY AS PS_PARTKEY /* + materialize */,
      F0_0.VALUE AS VALUE,
      F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
      F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
      F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
      F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
      F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
      F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY"
    FROM
      (
        SELECT
          F0_0.PS_PARTKEY AS PS_PARTKEY,
          F0_0.VALUE AS VALUE,
          F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
          F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
          F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
          F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
          F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
          F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
          F0_0._RESULT_TID AS _RESULT_TID,
          F0_0._SETPROV_DUP_COUNT AS _SETPROV_DUP_COUNT
        FROM
          (
            SELECT
              F0_0.PS_PARTKEY AS PS_PARTKEY,
              F0_0.VALUE AS VALUE,
              F0_0."(SUM((PS_SUPPLYCOST*PS_AVAILQTY))*0000010)" AS "(SUM((PS_SUPPLYCOST*PS_AVAILQTY))*0000010)",
              F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
              F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
              F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
              F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
              F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
              F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
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
            F0_0.VALUE > F0_0."(SUM((PS_SUPPLYCOST*PS_AVAILQTY))*0000010)"
          )
        ORDER BY
          VALUE DESC
      ) AS F0_0
  )
SELECT
  F0_0.PROV_PARTSUPP_PS__PARTKEY AS PROV_PARTSUPP_PS__PARTKEY,
  F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
  F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
  F0_0."PROV_PARTSUPP_1_PS__PARTKEY" AS "PROV_PARTSUPP_1_PS__PARTKEY",
  F0_0."PROV_SUPPLIER_1_S__SUPPKEY" AS "PROV_SUPPLIER_1_S__SUPPKEY",
  F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
  F0_0.PS_PARTKEY AS PS_PARTKEY
FROM
  (
    SELECT
      *
    FROM
      temp_view_0
  ) AS F0_0
