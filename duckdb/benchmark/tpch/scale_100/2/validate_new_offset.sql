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
            and (
                partsupp.rowid,
                supplier.rowid,
                nation.rowid,
                region.rowid
            ) in (
                select (
                        column_1_1,
                        column_2_1,
                        column_3_1,
                        column_4_1
                    )
                from LAYER_1_%OUT_ID%
            )
    )
    and (
        part.rowid,
        supplier.rowid,
        partsupp.rowid,
        nation.rowid,
        region.rowid
    ) in (
        SELECT (eval(column_0, column_1), eval(column_1, column_2), eval(column_2, column_3), eval(column_3, column_4), eval(column_4, column_5))
        from LAYER_3_%OUT_ID%
    )
order by s_acctbal desc,
    n_name,
    s_name,
    p_partkey
LIMIT 100;