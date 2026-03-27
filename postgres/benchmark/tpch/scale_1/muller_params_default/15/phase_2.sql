-- using default substitutions
-- $ID$
-- TPC-H/TPC-R Top Supplier Query (Q15)
-- Functional Query Definition
-- Approved February 1998


select
    _t.tuid as tuid,
    provSize(s_suppkey) as s_suppkey,
    provSize(s_name) as s_name,
    provSize(s_address) as s_address,
    provSize(s_phone) as s_phone,
    provSize(total_revenue) as total_revenue
from
    (
        select
            _j.tuid as tuid,
            s_suppkey as s_suppkey,
            s_name as s_name,
            s_address as s_address,
            s_phone as s_phone,
            total_revenue as total_revenue
        from
        	supplier_2 s,
        	revenue0_2 r
            , readJoin(4, s.tuid, r.tuid) _j
    ) as _t
    , readOrderBy(3, _t.tuid) _o
order by
    _o.sequence