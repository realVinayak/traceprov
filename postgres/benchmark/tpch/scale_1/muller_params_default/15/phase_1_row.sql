-- using default substitutions
-- $ID$
-- TPC-H/TPC-R Top Supplier Query (Q15)
-- Functional Query Definition
-- Approved February 1998


select
    writeOrderBy(3, _t.tuid, row_number() over _order) as tuid,
    s_suppkey as s_suppkey,
    s_name as s_name,
    s_address as s_address,
    s_phone as s_phone,
    total_revenue as total_revenue
from
    (
        select
            writeJoin(4, s.tuid, r.tuid) as tuid,
            s_suppkey as s_suppkey,
            s_name as s_name,
            s_address as s_address,
            s_phone as s_phone,
            total_revenue as total_revenue
        from
        	supplier_1 s,
        	revenue0_1 r
        where
        	s_suppkey = supplier_no
        	and total_revenue = (
        		select
        			max(total_revenue)
        		from
        			revenue0_1 r
                group by
                    ()
        	)
    ) as _t
window
    _order as (
        order by
            s_suppkey
    )
order by
    row_number() over _order