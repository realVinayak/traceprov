-- using default substitutions
-- $ID$
-- TPC-H/TPC-R Large Volume Customer Query (Q18)
-- Function Query Definition
-- Approved February 1998


select
    writeOrderBy(1, _t.tuid, row_number() over _order) as tuid,
    c_name as c_name,
    c_custkey as c_custkey,
    o_orderkey as o_orderkey,
    o_orderdate as o_orderdate,
    o_totalprice as o_totalprice,
    sum as sum
from
    (
        select
            writeAggregation(2, array_agg(_t.tuid)) as tuid,
        	c_name as c_name,
        	c_custkey as c_custkey,
        	o_orderkey as o_orderkey,
        	o_orderdate as o_orderdate,
        	o_totalprice as o_totalprice,
        	sum(l_quantity) as sum
        from
            (
                select
                    writeJoin(3, c.tuid, o.tuid, l.tuid) as tuid,
                    c_name as c_name,
                    c_custkey as c_custkey,
                    o_orderkey as o_orderkey,
                    o_orderdate as o_orderdate,
                    o_totalprice as o_totalprice,
                    l_quantity as l_quantity
                from
                	customer_1 c,
                	orders_1 o,
                	lineitem_1 l
                where
                	o_orderkey in (
                		select
                			l_orderkey as l_orderkey
                        from
                            (
                                select
                                    writeAggregation(4, array_agg(l2.tuid)) as tuid,
                                    l_orderkey as l_orderkey
                                from
                        			lineitem_1 l2
                        		group by
                        			l_orderkey 
                                having
                    				sum(l_quantity) > 300
                            ) as _t
                	)
                	and c_custkey = o_custkey
                	and o_orderkey = l_orderkey
            ) as _t
        group by
        	c_name,
        	c_custkey,
        	o_orderkey,
        	o_orderdate,
        	o_totalprice
    ) as _t
window
    _order as (
        order by
            o_totalprice desc,
            o_orderdate
    )
order by
    row_number() over _order