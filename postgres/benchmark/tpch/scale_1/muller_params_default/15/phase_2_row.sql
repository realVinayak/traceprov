-- using default substitutions
-- $ID$
-- TPC-H/TPC-R Top Supplier Query (Q15)
-- Functional Query Definition
-- Approved February 1998
select _t.tuid as tuid,
    s_suppkey as s_suppkey,
    l_orderkey as l_orderkey
from (
        select _j.tuid as tuid,
            s_suppkey as s_suppkey,
            l_orderkey as l_orderkey
        from supplier_2_row s,
            revenue0_2_row r,
            readJoin(4, s.tuid, r.tuid) _j
    ) as _t,
    readOrderBy(3, _t.tuid) _o
order by _o.sequence