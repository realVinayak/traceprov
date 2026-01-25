SELECT
    tp_table_2.cntrycode,
    tp_table_2.numcust,
    tp_table_2.totacctbal,
    tp_table_2.mapped_agg,
    traceprov_log_entry_1 (4, tp_table_2.mapped_agg) AS tp_table_3
FROM
    (
        SELECT
            custsale.cntrycode,
            count(*) AS numcust,
            sum(custsale.c_acctbal) AS totacctbal,
            traceprov_agg_key_parallel_offset_1 (3, (custsale.tp_c_custkey)::bigint) AS mapped_agg
        FROM
            (
                SELECT
                    SUBSTRING(
                        customer.c_phone
                        FROM
                            1 FOR 2
                    ) AS cntrycode,
                    customer.c_acctbal,
                    customer.c_custkey AS tp_c_custkey
                FROM
                    customer
                WHERE
                    (
                        (
                            SUBSTRING(
                                customer.c_phone
                                FROM
                                    1 FOR 2
                            ) = ANY (
                                ARRAY[
                                    '13'::text,
                                    '31'::text,
                                    '23'::text,
                                    '29'::text,
                                    '30'::text,
                                    '18'::text,
                                    '17'::text
                                ]
                            )
                        )
                        AND (
                            customer.c_acctbal > (
                                SELECT
                                    tp_table_1.avg
                                FROM
                                    (
                                        SELECT
                                            avg(customer_1.c_acctbal) AS avg,
                                            traceprov_log_entry_volatile_1 (
                                                2,
                                                traceprov_agg_key_parallel_offset_1 (1, (customer_1.c_custkey)::bigint)
                                            ) AS tp_table_0
                                        FROM
                                            customer customer_1
                                        WHERE
                                            (
                                                (customer_1.c_acctbal > 0.00)
                                                AND (
                                                    SUBSTRING(
                                                        customer_1.c_phone
                                                        FROM
                                                            1 FOR 2
                                                    ) = ANY (
                                                        ARRAY[
                                                            '13'::text,
                                                            '31'::text,
                                                            '23'::text,
                                                            '29'::text,
                                                            '30'::text,
                                                            '18'::text,
                                                            '17'::text
                                                        ]
                                                    )
                                                )
                                            )
                                    ) tp_table_1
                            )
                        )
                        AND (
                            NOT (
                                EXISTS (
                                    SELECT
                                        orders.o_orderkey,
                                        orders.o_custkey,
                                        orders.o_orderstatus,
                                        orders.o_totalprice,
                                        orders.o_orderdate,
                                        orders.o_orderpriority,
                                        orders.o_clerk,
                                        orders.o_shippriority,
                                        orders.o_comment
                                    FROM
                                        orders
                                    WHERE
                                        (orders.o_custkey = customer.c_custkey)
                                )
                            )
                        )
                    )
            ) custsale
        GROUP BY
            custsale.cntrycode
        ORDER BY
            custsale.cntrycode
    ) tp_table_2