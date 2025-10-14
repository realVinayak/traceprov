PROVENANCE OF (
    SELECT ps_partkey, value FROM (
    select
        ps_partkey,
        sum(ps_supplycost * ps_availqty) as value
    from
        partsupp USE PROVENANCE (ps_partkey, ps_suppkey),
        supplier USE PROVENANCE (s_suppkey),
        nation USE PROVENANCE (n_nationkey)
    where
        ps_suppkey = s_suppkey
        and s_nationkey = n_nationkey
        and n_name = 'GERMANY'
    group by
        ps_partkey 
    ) a JOIN (
        SELECT 
            sum(ps_supplycost * ps_availqty) * 0.0001000000 as computed_value
        FROM
            partsupp USE PROVENANCE (ps_partkey, ps_suppkey),
            supplier USE PROVENANCE (s_suppkey),
            nation USE PROVENANCE (n_nationkey)
        WHERE 
            ps_suppkey = s_suppkey
            and s_nationkey = n_nationkey
            and n_name = 'GERMANY'
    ) b
    on a.value > b.computed_value
    order by
        value desc
);

