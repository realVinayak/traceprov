WITH
  temp_view_1 AS (
    SELECT
      /*+ materialize */ F0_0."AGGR_0" AS "AGGR_0",
      F0_0."AGGR_1" AS "AGGR_1",
      F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F1_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY
    FROM
      (
        (
          SELECT
            SUM(
              (
                CASE
                  WHEN (F1_0.P_TYPE LIKE 'PROMO%') THEN (F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT))
                  ELSE 0
                END
              )
            ) AS "AGGR_0",
            SUM((F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT))) AS "AGGR_1"
          FROM
            (
              LINEITEM F0_0
              CROSS JOIN PART F1_0
            )
          WHERE
            (
              (
                (F0_0.L_PARTKEY = F1_0.P_PARTKEY)
                AND (F0_0.L_SHIPDATE >= '1995-09-01')
              )
              AND (F0_0.L_SHIPDATE < '1995-10-01')
            )
        ) F0_0
        LEFT OUTER JOIN (
          SELECT
            (
              CASE
                WHEN (F0_0.P_TYPE LIKE 'PROMO%') THEN (F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT))
                ELSE 0
              END
            ) AS "AGG_GB_ARG0",
            (F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT)) AS "AGG_GB_ARG1",
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
            F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY
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
                F1_0.P_PARTKEY AS P_PARTKEY,
                F1_0.P_NAME AS P_NAME,
                F1_0.P_MFGR AS P_MFGR,
                F1_0.P_BRAND AS P_BRAND,
                F1_0.P_TYPE AS P_TYPE,
                F1_0.P_SIZE AS P_SIZE,
                F1_0.P_CONTAINER AS P_CONTAINER,
                F1_0.P_RETAILPRICE AS P_RETAILPRICE,
                F1_0.P_COMMENT AS P_COMMENT,
                F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                F1_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY
              FROM
                (
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
                      F0_0.L_ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
                    FROM
                      LINEITEM F0_0
                  ) F0_0
                  CROSS JOIN (
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
                      F0_0.P_PARTKEY AS PROV_PART_P__PARTKEY
                    FROM
                      PART F0_0
                  ) F1_0
                )
            ) F0_0
          WHERE
            (
              (
                (F0_0.L_PARTKEY = F0_0.P_PARTKEY)
                AND (F0_0.L_SHIPDATE >= '1995-09-01')
              )
              AND (F0_0.L_SHIPDATE < '1995-10-01')
            )
        ) F1_0 ON ((1 = 1))
      )
  ),
  temp_view_0 AS (
    SELECT
      /*+ materialize */ ((100.000000 * F0_0."AGGR_0") / F0_0."AGGR_1") AS PROMO_REVENUE,
      F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
      F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY
    FROM
      (
        SELECT
          *
        FROM
          temp_view_1
      ) F0_0
  )
SELECT
  F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
  F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY
FROM
  (
    SELECT
      *
    FROM
      temp_view_0
  ) F0_0;
