-- using default substitutions
-- $ID$
-- TPC-H/TPC-R Customer Distribution Query (Q13)
-- Functional Query Definition
-- Approved February 1998


select
    writeOrderBy(1, _t.tuid, row_number() over _order) as tuid,
    c_count as c_count,
    custdist as custdist
from
    (
        select
            writeAggregation(2, array_agg(c_orders.tuid)) as tuid,
        	c_count as c_count,
        	count(*) as custdist
        from
        	(
                select
                    writeAggregation(3, array_agg(_t.tuid)) as tuid,
                    c_custkey as c_custkey,
                    count(o_orderkey) as c_count
                from
                    (
                		select
                            writeJoin(4, c.tuid, o.tuid) as tuid,
                			c_custkey as c_custkey,
                			o_orderkey as o_orderkey
                		from
                			customer_1 c left outer join orders_1 o on
                				c_custkey = o_custkey
                				and o_comment not like '%special%requests%'
                    ) as _t
        		group by
        			c_custkey
        	) as c_orders
        group by
        	c_count
    ) as _t
window
    _order as (
        order by
            custdist desc,
            c_count desc
    )
order by
    row_number() over _order