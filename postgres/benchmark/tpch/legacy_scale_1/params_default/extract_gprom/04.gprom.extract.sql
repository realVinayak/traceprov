
SELECT prov_orders_o__orderkey,
    prov_lineitem_l__linenumber,
    prov_lineitem_l__orderkey
FROM (
        PROVENANCE OF (
            select o_orderpriority,
                count(*) as order_count
            from orders USE PROVENANCE (o_orderkey)
                join (
                    SELECT 1,
                        l_orderkey
                    FROM lineitem USE PROVENANCE (l_linenumber, l_orderkey)
                    WHERE l_commitdate < l_receiptdate
                    group by l_orderkey
                ) f ON o_orderkey = l_orderkey
            where o_orderdate >= '1993-07-01'
                and o_orderdate < '1993-10-01'
            group by o_orderpriority
            order by o_orderpriority
        )
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