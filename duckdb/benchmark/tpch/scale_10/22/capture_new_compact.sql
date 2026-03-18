-- using default substitutions
select *,
    traceprov_log_entry_1 (4, mapped_agg)
FROM (
        select cntrycode,
            count(*) as numcust,
            sum(c_acctbal) as totacctbal,
            traceprov_agg_key_parallel_offset_1 (3, (custsale.rowid::int)) as mapped_agg
        from (
                select substring(
                        c_phone
                        from 1 for 2
                    ) as cntrycode,
                    c_acctbal,
                    customer.rowid
                from customer
                where substring(
                        c_phone
                        from 1 for 2
                    ) in ('13', '31', '23', '29', '30', '18', '17')
                    and c_acctbal > (
                        select avged
                        from (
                                select avg(c_acctbal) as avged,
                                    traceprov_log_entry_volatile_1 (
                                        2,
                                        traceprov_agg_key_parallel_offset_1 (1, customer.rowid::int)
                                    )
                                from customer
                                where c_acctbal > 0.00
                                    and substring(
                                        c_phone
                                        from 1 for 2
                                    ) in ('13', '31', '23', '29', '30', '18', '17')
                            )
                    )
                    and not exists (
                        select 1
                        from orders
                        where o_custkey = c_custkey
                    )
            ) as custsale
        group by cntrycode
        order by cntrycode
    ) F;