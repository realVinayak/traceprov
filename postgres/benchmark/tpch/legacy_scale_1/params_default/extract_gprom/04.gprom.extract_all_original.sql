PROVENANCE OF (
    select o_orderpriority,
        count(*) as order_count
    from orders USE PROVENANCE (o_orderkey)
    where o_orderdate >= '1993-07-01'
        and o_orderdate < '1993-10-01'
        and exists (
            select *
            from lineitem USE PROVENANCE (l_orderkey, l_linenumber)
            where l_orderkey = o_orderkey
                and l_commitdate < l_receiptdate
        )
    group by o_orderpriority
    order by o_orderpriority
);
-- NOT DONE --
-- -- using default substitutions
-- select
-- 	o_orderpriority,
-- 	count(*) as order_count
-- from
-- 	orders
-- where
-- 	o_orderdate >= date '1993-07-01'
-- 	and o_orderdate < date '1993-07-01' + interval '3' month
-- 	and exists (
-- 		select
-- 			*
-- 		from
-- 			lineitem
-- 		where
-- 			l_orderkey = o_orderkey
-- 			and l_commitdate < l_receiptdate
-- 	)
-- group by
-- 	o_orderpriority
-- order by
-- 	o_orderpriority;