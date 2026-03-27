-- using default substitutions
-- $ID$
-- TPC-H/TPC-R Parts/Supplier Relationship Query (Q16)
-- Functional Query Definition
-- Approved February 1998


-- optimizations:
-- * skip read|writeFilter()


select
    _t.tuid as tuid,
    provSize(p_brand) as p_brand,
    provSize(p_type) as p_type,
    provSize(p_size) as p_size,
    provSize(supplier_cnt) as supplier_cnt
from
    (
        select
            _g.tuid as tuid,
            concat_agg(p_brand) as p_brand,
            concat_agg(p_type) as p_type,
            concat_agg(p_size) as p_size,
            concat_agg(ps_suppkey) as supplier_cnt
        from
            (
                select
                    _j.tuid as tuid,
                    p_brand as p_brand,
                    p_type as p_type,
                    p_size as p_size,
                    ps_suppkey as ps_suppkey
                from
                	partsupp_2 ps,
                	part_2 p
                    , readJoin(3, ps.tuid, p.tuid) _j
            ) as _t
            , readAggregation(2, _t.tuid) _g
        group by
        	_g.tuid
    ) as _t
    , readOrderBy(1, _t.tuid) _o
order by
    _o.sequence