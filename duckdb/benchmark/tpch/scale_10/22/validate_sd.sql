-- using default substitutions
select cntrycode,
    count(*) as numcust,
    sum(c_acctbal) as totacctbal
from (
        select substring(
                c_phone
                from 1 for 2
            ) as cntrycode,
            c_acctbal
        from customer
        where customer.rowid in (
                select iid from LAYER_1_SD_%OUT_ID% where "table" = 7
            )
            and c_acctbal > (
                select avg(c_acctbal)
                from customer
                where (customer.rowid) in (
                        select iid from LAYER_1_SD_%OUT_ID% where "table" = 14
                    )
            )
            and not exists (
                select *
                from orders
                where o_custkey = c_custkey
            )
    ) as custsale
group by cntrycode
order by cntrycode;