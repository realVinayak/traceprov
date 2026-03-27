-- using default substitutions
-- $ID$
-- TPC-H/TPC-R Large Volume Customer Query (Q18)
-- Function Query Definition
-- Approved February 1998


select
    _t.tuid as tuid,
    provSize(c_name) as c_name,
    provSize(c_custkey) as c_custkey,
    provSize(o_orderkey) as o_orderkey,
    provSize(o_orderdate) as o_orderdate,
    provSize(o_totalprice) as o_totalprice,
    provSize(sum) as sum
from
    (
        select
            _g.tuid as tuid,
        	concat_agg(c_name) as c_name,
        	concat_agg(c_custkey) as c_custkey,
        	concat_agg(o_orderkey) as o_orderkey,
        	concat_agg(o_orderdate) as o_orderdate,
        	concat_agg(o_totalprice) as o_totalprice,
        	concat_agg(l_quantity) as sum
        from
            (
                select
                    _j.tuid as tuid,
                    c_name as c_name,
                    c_custkey as c_custkey,
                    o_orderkey as o_orderkey,
                    o_orderdate as o_orderdate,
                    o_totalprice as o_totalprice,
                    l_quantity as l_quantity
                from
                	customer_2 c,
                	orders_2 o,
                	lineitem_2 l
                    , readJoin(3, c.tuid, o.tuid, l.tuid) _j
            ) as _t
        , readAggregation(2, _t.tuid) _g
        group by
        	_g.tuid
    ) as _t
    , readOrderBy(1, _t.tuid) _o
order by
    _o.sequence