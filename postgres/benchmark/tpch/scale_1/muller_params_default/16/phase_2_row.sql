-- using default substitutions
-- $ID$
-- TPC-H/TPC-R Parts/Supplier Relationship Query (Q16)
-- Functional Query Definition
-- Approved February 1998
-- optimizations:
-- * skip read|writeFilter()
select _t.tuid as tuid,
    p_partkey,
    ps_partkey
from (
        select _g.tuid as tuid,
            concat_agg(p_partkey) as p_partkey,
            concat_agg(ps_partkey) as ps_partkey
        from (
                select _j.tuid as tuid,
                    ps_partkey as ps_partkey,
                    p_partkey as p_partkey
                from partsupp_2_row ps,
                    part_2_row p,
                    readJoin(3, ps.tuid, p.tuid) _j
            ) as _t,
            readAggregation(2, _t.tuid) _g
        group by _g.tuid
    ) as _t,
    readOrderBy(1, _t.tuid) _o
order by _o.sequence