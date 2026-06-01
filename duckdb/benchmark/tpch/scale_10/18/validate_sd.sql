-- using default substitutions
select c_name,
    c_custkey,
    o_orderkey,
    o_orderdate,
    o_totalprice,
    sum(l_quantity)
from customer,
    orders,
    lineitem
where c_custkey = o_custkey
    and o_orderkey = l_orderkey
	and o_orderkey in (
		select
			l_orderkey
		from
			lineitem
		group by
			l_orderkey having
				sum(l_quantity) > 300
	)
    and customer.rowid in (
        select iid
        from LAYER_1_SD_%OUT_ID%
        where "table" = 2
    )
    and lineitem.rowid in (
        select iid
        from LAYER_1_SD_%OUT_ID%
        where "table" = 0
    )
group by c_name,
    c_custkey,
    o_orderkey,
    o_orderdate,
    o_totalprice
order by o_totalprice desc,
    o_orderdate
LIMIT 100;