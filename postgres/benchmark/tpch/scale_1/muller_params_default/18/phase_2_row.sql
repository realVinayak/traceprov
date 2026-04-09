-- using default substitutions
-- $ID$
-- TPC-H/TPC-R Large Volume Customer Query (Q18)
-- Function Query Definition
-- Approved February 1998
select _t.tuid as tuid,
    c_custkey,
    o_orderkey,
    l_orderkey
from (
        select _g.tuid as tuid,
            concat_agg(c_custkey) as c_custkey,
            concat_agg(o_orderkey) as o_orderkey,
            concat_agg(l_orderkey) as l_orderkey
        from (
                select _j.tuid as tuid,
                    c_custkey as c_custkey,
                    o_orderkey as o_orderkey,
                    l_orderkey as l_orderkey
                from customer_2_row c,
                    orders_2_row o,
                    lineitem_2_row l,
                    readJoin(3, c.tuid, o.tuid, l.tuid) _j
            ) as _t,
            readAggregation(2, _t.tuid) _g
        group by _g.tuid
    ) as _t,
    readOrderBy(1, _t.tuid) _o
order by _o.sequence