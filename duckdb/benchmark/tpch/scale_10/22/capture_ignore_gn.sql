-- using default substitutions
select
    *
FROM
    (
        select
            cntrycode,
            count(*) as numcust,
            sum(c_acctbal) as totacctbal,
            traceprov_agg_key_parallel_offset_ignore_gn_1 (3, (custsale.rowid)) as mapped_agg
        from
            (
                select
                    substring(
                        c_phone
                        from
                            1 for 2
                    ) as cntrycode,
                    c_acctbal,
                    customer.rowid
                from
                    customer
                where
                    substring(
                        c_phone
                        from
                            1 for 2
                    ) in ('13', '31', '23', '29', '30', '18', '17')
                    and c_acctbal > (
                        select
                            avged
                        from
                            (
                                select
                                    avg(c_acctbal) as avged,
                                    traceprov_agg_key_parallel_offset_ignore_gn_1 (1, customer.rowid)
                                from
                                    customer
                                where
                                    c_acctbal > 0.00
                                    and substring(
                                        c_phone
                                        from
                                            1 for 2
                                    ) in ('13', '31', '23', '29', '30', '18', '17')
                            )
                    )
                    and not exists (
                        select
                            1
                        from
                            orders
                        where
                            o_custkey = c_custkey
                    )
            ) as custsale
        group by
            cntrycode
        order by
            cntrycode
    ) F;