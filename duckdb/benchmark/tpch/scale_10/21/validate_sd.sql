-- using default substitutions
select s_name,
    count(*) as numwait
from (select * from supplier where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 18)) supplier,
    (select * from lineitem where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 16)) l1,
    (select * from orders where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 15)) orders,
    (select * from nation where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 19)) nation
where exists (
		select
			*
		from
			lineitem l2
		where
			l2.l_orderkey = l1.l_orderkey
			and l2.l_suppkey <> l1.l_suppkey
    )
    and not exists (
        select *
        from lineitem l3
        where l3.l_orderkey = l1.l_orderkey
            and l3.l_suppkey <> l1.l_suppkey
            and l3.l_receiptdate > l3.l_commitdate
    )
    and s_suppkey = l1.l_suppkey
    and o_orderkey = l1.l_orderkey
    and s_nationkey = n_nationkey
group by s_name
order by numwait desc,
    s_name
LIMIT 100