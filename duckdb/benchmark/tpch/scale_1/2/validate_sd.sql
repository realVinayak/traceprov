-- using default substitutions
select s_acctbal,
    s_name,
    n_name,
    p_partkey,
    p_mfgr,
    s_address,
    s_phone,
    s_comment
from part,
    supplier,
    partsupp,
    nation,
    region
where p_partkey = ps_partkey
    and s_suppkey = ps_suppkey
    and s_nationkey = n_nationkey
    and n_regionkey = r_regionkey
    and ps_supplycost = (
        select min(ps_supplycost)
        from partsupp,
            supplier,
            nation,
            region
        where p_partkey = ps_partkey
            and s_suppkey = ps_suppkey
            and s_nationkey = n_nationkey
            and n_regionkey = r_regionkey
			and r_name = 'EUROPE'
    )
    and part.rowid in (
            select iid
            from LAYER_1_SD_%OUT_ID%
            where "table" = 18
        )
    and supplier.rowid in (
            select iid
            from LAYER_1_SD_%OUT_ID%
            where "table" = 21
        )
    and partsupp.rowid in (
            select iid
            from LAYER_1_SD_%OUT_ID%
            where "table" = 17
        )
    and nation.rowid in (
            select iid
            from LAYER_1_SD_%OUT_ID%
            where "table" = 22
        )
    and region.rowid in (
            select iid
            from LAYER_1_SD_%OUT_ID%
            where "table" = 23
        )
order by s_acctbal desc,
    n_name,
    s_name,
    p_partkey
LIMIT 100;