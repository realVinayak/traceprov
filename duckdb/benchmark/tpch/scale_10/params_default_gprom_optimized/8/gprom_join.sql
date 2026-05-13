WITH temp_view_1 AS (
    SELECT
        /*+ materialize */
        DATE_PART('YEAR', (F3_0.O_ORDERDATE)::DATE) AS O_YEAR,
        (F2_0.L_EXTENDEDPRICE * (1 - F2_0.L_DISCOUNT)) AS VOLUME,
        F6_0.N_NAME AS NATION
    FROM (
            (
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
                            CROSS JOIN ORDERS F3_0
                        )
                        CROSS JOIN CUSTOMER F4_0
                    )
                    CROSS JOIN NATION F5_0
                )
                CROSS JOIN NATION F6_0
            )
            CROSS JOIN REGION F7_0
        )
    WHERE (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            (
                                                (F0_0.P_PARTKEY = F2_0.L_PARTKEY)
                                                AND (F1_0.S_SUPPKEY = F2_0.L_SUPPKEY)
                                            )
                                            AND (F2_0.L_ORDERKEY = F3_0.O_ORDERKEY)
                                        )
                                        AND (F3_0.O_CUSTKEY = F4_0.C_CUSTKEY)
                                    )
                                    AND (F4_0.C_NATIONKEY = F5_0.N_NATIONKEY)
                                )
                                AND (F5_0.N_REGIONKEY = F7_0.R_REGIONKEY)
                            )
                            AND (F7_0.R_NAME = 'AMERICA')
                        )
                        AND (F1_0.S_NATIONKEY = F6_0.N_NATIONKEY)
                    )
                    AND (F3_0.O_ORDERDATE >= '1995-01-01')
                )
                AND (F3_0.O_ORDERDATE <= '1996-12-31')
            )
            AND (F0_0.P_TYPE = 'ECONOMY ANODIZED STEEL')
        )
),
temp_view_3 AS (
    SELECT
        /*+ materialize */
        DATE_PART('YEAR', (F0_0.O_ORDERDATE)::DATE) AS O_YEAR,
        (F0_0.L_EXTENDEDPRICE * (1 - F0_0.L_DISCOUNT)) AS VOLUME,
        F0_0."N_NAME1" AS NATION,
        F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
        F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
        F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
        F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
        F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
        F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
        F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
        F0_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY
    FROM (
            SELECT F0_0.P_PARTKEY AS P_PARTKEY,
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
                F0_0.O_ORDERKEY AS O_ORDERKEY,
                F0_0.O_CUSTKEY AS O_CUSTKEY,
                F0_0.O_ORDERSTATUS AS O_ORDERSTATUS,
                F0_0.O_TOTALPRICE AS O_TOTALPRICE,
                F0_0.O_ORDERDATE AS O_ORDERDATE,
                F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
                F0_0.O_CLERK AS O_CLERK,
                F0_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
                F0_0.O_COMMENT AS O_COMMENT,
                F0_0.C_CUSTKEY AS C_CUSTKEY,
                F0_0.C_NAME AS C_NAME,
                F0_0.C_ADDRESS AS C_ADDRESS,
                F0_0.C_NATIONKEY AS C_NATIONKEY,
                F0_0.C_PHONE AS C_PHONE,
                F0_0.C_ACCTBAL AS C_ACCTBAL,
                F0_0.C_MKTSEGMENT AS C_MKTSEGMENT,
                F0_0.C_COMMENT AS C_COMMENT,
                F0_0.N_NATIONKEY AS N_NATIONKEY,
                F0_0.N_NAME AS N_NAME,
                F0_0.N_REGIONKEY AS N_REGIONKEY,
                F0_0.N_COMMENT AS N_COMMENT,
                F0_0."N_NATIONKEY1" AS "N_NATIONKEY1",
                F0_0."N_NAME1" AS "N_NAME1",
                F0_0."N_REGIONKEY1" AS "N_REGIONKEY1",
                F0_0."N_COMMENT1" AS "N_COMMENT1",
                F1_0.R_REGIONKEY AS R_REGIONKEY,
                F1_0.R_NAME AS R_NAME,
                F1_0.R_COMMENT AS R_COMMENT,
                F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
                F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
                F1_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY
            FROM (
                    (
                        SELECT F0_0.P_PARTKEY AS P_PARTKEY,
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
                            F0_0.O_ORDERKEY AS O_ORDERKEY,
                            F0_0.O_CUSTKEY AS O_CUSTKEY,
                            F0_0.O_ORDERSTATUS AS O_ORDERSTATUS,
                            F0_0.O_TOTALPRICE AS O_TOTALPRICE,
                            F0_0.O_ORDERDATE AS O_ORDERDATE,
                            F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
                            F0_0.O_CLERK AS O_CLERK,
                            F0_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
                            F0_0.O_COMMENT AS O_COMMENT,
                            F0_0.C_CUSTKEY AS C_CUSTKEY,
                            F0_0.C_NAME AS C_NAME,
                            F0_0.C_ADDRESS AS C_ADDRESS,
                            F0_0.C_NATIONKEY AS C_NATIONKEY,
                            F0_0.C_PHONE AS C_PHONE,
                            F0_0.C_ACCTBAL AS C_ACCTBAL,
                            F0_0.C_MKTSEGMENT AS C_MKTSEGMENT,
                            F0_0.C_COMMENT AS C_COMMENT,
                            F0_0.N_NATIONKEY AS N_NATIONKEY,
                            F0_0.N_NAME AS N_NAME,
                            F0_0.N_REGIONKEY AS N_REGIONKEY,
                            F0_0.N_COMMENT AS N_COMMENT,
                            F1_0.N_NATIONKEY AS "N_NATIONKEY1",
                            F1_0.N_NAME AS "N_NAME1",
                            F1_0.N_REGIONKEY AS "N_REGIONKEY1",
                            F1_0.N_COMMENT AS "N_COMMENT1",
                            F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                            F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                            F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                            F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                            F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
                            F1_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY"
                        FROM (
                                (
                                    SELECT F0_0.P_PARTKEY AS P_PARTKEY,
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
                                        F0_0.O_ORDERKEY AS O_ORDERKEY,
                                        F0_0.O_CUSTKEY AS O_CUSTKEY,
                                        F0_0.O_ORDERSTATUS AS O_ORDERSTATUS,
                                        F0_0.O_TOTALPRICE AS O_TOTALPRICE,
                                        F0_0.O_ORDERDATE AS O_ORDERDATE,
                                        F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
                                        F0_0.O_CLERK AS O_CLERK,
                                        F0_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
                                        F0_0.O_COMMENT AS O_COMMENT,
                                        F0_0.C_CUSTKEY AS C_CUSTKEY,
                                        F0_0.C_NAME AS C_NAME,
                                        F0_0.C_ADDRESS AS C_ADDRESS,
                                        F0_0.C_NATIONKEY AS C_NATIONKEY,
                                        F0_0.C_PHONE AS C_PHONE,
                                        F0_0.C_ACCTBAL AS C_ACCTBAL,
                                        F0_0.C_MKTSEGMENT AS C_MKTSEGMENT,
                                        F0_0.C_COMMENT AS C_COMMENT,
                                        F1_0.N_NATIONKEY AS N_NATIONKEY,
                                        F1_0.N_NAME AS N_NAME,
                                        F1_0.N_REGIONKEY AS N_REGIONKEY,
                                        F1_0.N_COMMENT AS N_COMMENT,
                                        F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                                        F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                                        F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                                        F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                                        F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                                        F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY
                                    FROM (
                                            (
                                                SELECT F0_0.P_PARTKEY AS P_PARTKEY,
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
                                                    F0_0.O_ORDERKEY AS O_ORDERKEY,
                                                    F0_0.O_CUSTKEY AS O_CUSTKEY,
                                                    F0_0.O_ORDERSTATUS AS O_ORDERSTATUS,
                                                    F0_0.O_TOTALPRICE AS O_TOTALPRICE,
                                                    F0_0.O_ORDERDATE AS O_ORDERDATE,
                                                    F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
                                                    F0_0.O_CLERK AS O_CLERK,
                                                    F0_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
                                                    F0_0.O_COMMENT AS O_COMMENT,
                                                    F1_0.C_CUSTKEY AS C_CUSTKEY,
                                                    F1_0.C_NAME AS C_NAME,
                                                    F1_0.C_ADDRESS AS C_ADDRESS,
                                                    F1_0.C_NATIONKEY AS C_NATIONKEY,
                                                    F1_0.C_PHONE AS C_PHONE,
                                                    F1_0.C_ACCTBAL AS C_ACCTBAL,
                                                    F1_0.C_MKTSEGMENT AS C_MKTSEGMENT,
                                                    F1_0.C_COMMENT AS C_COMMENT,
                                                    F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                                                    F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                                                    F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                                                    F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                                                    F1_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY
                                                FROM (
                                                        (
                                                            SELECT F0_0.P_PARTKEY AS P_PARTKEY,
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
                                                                F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY
                                                            FROM (
                                                                    (
                                                                        SELECT F0_0.P_PARTKEY AS P_PARTKEY,
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
                                                                            F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY
                                                                        FROM (
                                                                                (
                                                                                    SELECT F0_0.P_PARTKEY AS P_PARTKEY,
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
                                                                                        F1_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY
                                                                                    FROM (
                                                                                            (
                                                                                                SELECT F0_0.P_PARTKEY AS P_PARTKEY,
                                                                                                    F0_0.P_NAME AS P_NAME,
                                                                                                    F0_0.P_MFGR AS P_MFGR,
                                                                                                    F0_0.P_BRAND AS P_BRAND,
                                                                                                    F0_0.P_TYPE AS P_TYPE,
                                                                                                    F0_0.P_SIZE AS P_SIZE,
                                                                                                    F0_0.P_CONTAINER AS P_CONTAINER,
                                                                                                    F0_0.P_RETAILPRICE AS P_RETAILPRICE,
                                                                                                    F0_0.P_COMMENT AS P_COMMENT,
                                                                                                    F0_0.rowid AS PROV_PART_P__PARTKEY
                                                                                                FROM PART F0_0
                                                                                            ) F0_0
                                                                                            CROSS JOIN (
                                                                                                SELECT F0_0.S_SUPPKEY AS S_SUPPKEY,
                                                                                                    F0_0.S_NAME AS S_NAME,
                                                                                                    F0_0.S_ADDRESS AS S_ADDRESS,
                                                                                                    F0_0.S_NATIONKEY AS S_NATIONKEY,
                                                                                                    F0_0.S_PHONE AS S_PHONE,
                                                                                                    F0_0.S_ACCTBAL AS S_ACCTBAL,
                                                                                                    F0_0.S_COMMENT AS S_COMMENT,
                                                                                                    F0_0.rowid AS PROV_SUPPLIER_S__SUPPKEY
                                                                                                FROM SUPPLIER F0_0
                                                                                            ) F1_0
                                                                                        )
                                                                                ) F0_0
                                                                                CROSS JOIN (
                                                                                    SELECT F0_0.L_ORDERKEY AS L_ORDERKEY,
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
                                                                                    FROM LINEITEM F0_0
                                                                                ) F1_0
                                                                            )
                                                                    ) F0_0
                                                                    CROSS JOIN (
                                                                        SELECT F0_0.O_ORDERKEY AS O_ORDERKEY,
                                                                            F0_0.O_CUSTKEY AS O_CUSTKEY,
                                                                            F0_0.O_ORDERSTATUS AS O_ORDERSTATUS,
                                                                            F0_0.O_TOTALPRICE AS O_TOTALPRICE,
                                                                            F0_0.O_ORDERDATE AS O_ORDERDATE,
                                                                            F0_0.O_ORDERPRIORITY AS O_ORDERPRIORITY,
                                                                            F0_0.O_CLERK AS O_CLERK,
                                                                            F0_0.O_SHIPPRIORITY AS O_SHIPPRIORITY,
                                                                            F0_0.O_COMMENT AS O_COMMENT,
                                                                            F0_0.rowid AS PROV_ORDERS_O__ORDERKEY
                                                                        FROM ORDERS F0_0
                                                                    ) F1_0
                                                                )
                                                        ) F0_0
                                                        CROSS JOIN (
                                                            SELECT F0_0.C_CUSTKEY AS C_CUSTKEY,
                                                                F0_0.C_NAME AS C_NAME,
                                                                F0_0.C_ADDRESS AS C_ADDRESS,
                                                                F0_0.C_NATIONKEY AS C_NATIONKEY,
                                                                F0_0.C_PHONE AS C_PHONE,
                                                                F0_0.C_ACCTBAL AS C_ACCTBAL,
                                                                F0_0.C_MKTSEGMENT AS C_MKTSEGMENT,
                                                                F0_0.C_COMMENT AS C_COMMENT,
                                                                F0_0.rowid AS PROV_CUSTOMER_C__CUSTKEY
                                                            FROM CUSTOMER F0_0
                                                        ) F1_0
                                                    )
                                            ) F0_0
                                            CROSS JOIN (
                                                SELECT F0_0.N_NATIONKEY AS N_NATIONKEY,
                                                    F0_0.N_NAME AS N_NAME,
                                                    F0_0.N_REGIONKEY AS N_REGIONKEY,
                                                    F0_0.N_COMMENT AS N_COMMENT,
                                                    F0_0.rowid AS PROV_NATION_N__NATIONKEY
                                                FROM NATION F0_0
                                            ) F1_0
                                        )
                                ) F0_0
                                CROSS JOIN (
                                    SELECT F0_0.N_NATIONKEY AS N_NATIONKEY,
                                        F0_0.N_NAME AS N_NAME,
                                        F0_0.N_REGIONKEY AS N_REGIONKEY,
                                        F0_0.N_COMMENT AS N_COMMENT,
                                        F0_0.rowid AS "PROV_NATION_1_N__NATIONKEY"
                                    FROM NATION F0_0
                                ) F1_0
                            )
                    ) F0_0
                    CROSS JOIN (
                        SELECT F0_0.R_REGIONKEY AS R_REGIONKEY,
                            F0_0.R_NAME AS R_NAME,
                            F0_0.R_COMMENT AS R_COMMENT,
                            F0_0.rowid AS PROV_REGION_R__REGIONKEY
                        FROM REGION F0_0
                    ) F1_0
                )
        ) F0_0
    WHERE (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            (
                                                (F0_0.P_PARTKEY = F0_0.L_PARTKEY)
                                                AND (F0_0.S_SUPPKEY = F0_0.L_SUPPKEY)
                                            )
                                            AND (F0_0.L_ORDERKEY = F0_0.O_ORDERKEY)
                                        )
                                        AND (F0_0.O_CUSTKEY = F0_0.C_CUSTKEY)
                                    )
                                    AND (F0_0.C_NATIONKEY = F0_0.N_NATIONKEY)
                                )
                                AND (F0_0.N_REGIONKEY = F0_0.R_REGIONKEY)
                            )
                            AND (F0_0.R_NAME = 'AMERICA')
                        )
                        AND (F0_0.S_NATIONKEY = F0_0."N_NATIONKEY1")
                    )
                    AND (F0_0.O_ORDERDATE >= '1995-01-01')
                )
                AND (F0_0.O_ORDERDATE <= '1996-12-31')
            )
            AND (F0_0.P_TYPE = 'ECONOMY ANODIZED STEEL')
        )
),
temp_view_2 AS (
    SELECT
        /*+ materialize */
        (
            CASE
                WHEN (F0_0.NATION = 'BRAZIL') THEN F0_0.VOLUME
                ELSE 0
            END
        ) AS "AGG_GB_ARG0",
        F0_0.VOLUME AS "AGG_GB_ARG1",
        F0_0.O_YEAR AS "AGG_GB_ARG2",
        F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
        F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
        F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
        F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
        F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
        F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
        F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
        F0_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY
    FROM (
            SELECT *
            FROM temp_view_3
        ) F0_0
),
temp_view_0 AS (
    SELECT
        /*+ materialize */
        F0_0."AGGR_0" AS "AGGR_0",
        F0_0."AGGR_1" AS "AGGR_1",
        F0_0."GROUP_0" AS "GROUP_0",
        F1_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
        F1_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
        F1_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
        F1_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
        F1_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
        F1_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
        F1_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
        F1_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY
    FROM (
            (
                SELECT SUM(
                        (
                            CASE
                                WHEN (F0_0.NATION = 'BRAZIL') THEN F0_0.VOLUME
                                ELSE 0
                            END
                        )
                    ) AS "AGGR_0",
                    SUM(F0_0.VOLUME) AS "AGGR_1",
                    F0_0.O_YEAR AS "GROUP_0"
                FROM (
                        SELECT *
                        FROM temp_view_1
                    ) F0_0
                GROUP BY F0_0.O_YEAR
            ) F0_0
            JOIN (
                SELECT F0_0."AGG_GB_ARG2" AS "_P_SIDE_GROUP_0",
                    F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
                    F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
                    F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
                    F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
                    F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
                    F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
                    F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
                    F0_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY
                FROM (
                        SELECT *
                        FROM temp_view_2
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
)
SELECT F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
    F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
    F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
    F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
    F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
    F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
    F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
    F0_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY
FROM (
        SELECT F0_0."GROUP_0" AS O_YEAR,
            (F0_0."AGGR_0" / F0_0."AGGR_1") AS MKT_SHARE,
            F0_0.PROV_PART_P__PARTKEY AS PROV_PART_P__PARTKEY,
            F0_0.PROV_SUPPLIER_S__SUPPKEY AS PROV_SUPPLIER_S__SUPPKEY,
            F0_0.PROV_LINEITEM_L__ORDERKEY AS PROV_LINEITEM_L__ORDERKEY,
            F0_0.PROV_ORDERS_O__ORDERKEY AS PROV_ORDERS_O__ORDERKEY,
            F0_0.PROV_CUSTOMER_C__CUSTKEY AS PROV_CUSTOMER_C__CUSTKEY,
            F0_0.PROV_NATION_N__NATIONKEY AS PROV_NATION_N__NATIONKEY,
            F0_0."PROV_NATION_1_N__NATIONKEY" AS "PROV_NATION_1_N__NATIONKEY",
            F0_0.PROV_REGION_R__REGIONKEY AS PROV_REGION_R__REGIONKEY
        FROM (
                SELECT *
                FROM temp_view_0
            ) F0_0
        ORDER BY O_YEAR ASC NULLS LAST
    ) F0_0;