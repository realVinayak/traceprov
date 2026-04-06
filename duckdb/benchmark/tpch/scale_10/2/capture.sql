SELECT
    tp_table_2.s_acctbal,
    tp_table_2.s_name,
    tp_table_2.n_name,
    tp_table_2.p_partkey,
    tp_table_2.p_mfgr,
    tp_table_2.s_address,
    tp_table_2.s_phone,
    tp_table_2.s_comment,
    tp_table_2.tp_p_partkey,
    tp_table_2.tp_s_suppkey,
    tp_table_2.tp_ps_partkey,
    tp_table_2.tp_ps_suppkey,
    tp_table_2.tp_n_nationkey,
    tp_table_2.tp_r_regionkey,
    traceprov_log_entry_6 (
        3,
        (tp_table_2.tp_p_partkey)::bigint,
        (tp_table_2.tp_s_suppkey)::bigint,
        (tp_table_2.tp_ps_partkey)::bigint,
        (tp_table_2.tp_ps_suppkey)::bigint,
        (tp_table_2.tp_n_nationkey)::bigint,
        (tp_table_2.tp_r_regionkey)::bigint
    ) AS tp_table_3
FROM
    (
        SELECT
            supplier.s_acctbal,
            supplier.s_name,
            nation.n_name,
            part.p_partkey,
            part.p_mfgr,
            supplier.s_address,
            supplier.s_phone,
            supplier.s_comment,
            part.p_partkey AS tp_p_partkey,
            supplier.s_suppkey AS tp_s_suppkey,
            partsupp.ps_partkey AS tp_ps_partkey,
            partsupp.ps_suppkey AS tp_ps_suppkey,
            nation.n_nationkey AS tp_n_nationkey,
            region.r_regionkey AS tp_r_regionkey
        FROM
            part,
            supplier,
            partsupp,
            nation,
            region
        WHERE
            (
                (part.p_partkey = partsupp.ps_partkey)
                AND (supplier.s_suppkey = partsupp.ps_suppkey)
                AND (part.p_size = 15)
                AND ((part.p_type)::text ~~ '%BRASS'::text)
                AND (supplier.s_nationkey = nation.n_nationkey)
                AND (nation.n_regionkey = region.r_regionkey)
                AND (region.r_name = 'EUROPE'::bpchar)
                AND (
                    partsupp.ps_supplycost = (
                        SELECT
                            tp_table_1.min
                        FROM
                            (
                                SELECT
                                    min(partsupp_1.ps_supplycost) AS min,
                                    traceprov_log_entry_volatile_2 (
                                        2,
                                        (part.p_partkey)::bigint,
                                        traceprov_agg_key_parallel_offset_5 (
                                            1,
                                            (partsupp_1.ps_partkey)::bigint,
                                            (partsupp_1.ps_suppkey)::bigint,
                                            (supplier_1.s_suppkey)::bigint,
                                            (nation_1.n_nationkey)::bigint,
                                            (region_1.r_regionkey)::bigint
                                        )
                                    ) AS tp_table_0
                                FROM
                                    partsupp partsupp_1,
                                    supplier supplier_1,
                                    nation nation_1,
                                    region region_1
                                WHERE
                                    (
                                        (part.p_partkey = partsupp_1.ps_partkey)
                                        AND (supplier_1.s_suppkey = partsupp_1.ps_suppkey)
                                        AND (supplier_1.s_nationkey = nation_1.n_nationkey)
                                        AND (nation_1.n_regionkey = region_1.r_regionkey)
                                        AND (region_1.r_name = 'EUROPE'::bpchar)
                                    )
                            ) tp_table_1
                    )
                )
            )
        ORDER BY
            supplier.s_acctbal DESC,
            nation.n_name,
            supplier.s_name,
            part.p_partkey
        LIMIT
            100
    ) tp_table_2