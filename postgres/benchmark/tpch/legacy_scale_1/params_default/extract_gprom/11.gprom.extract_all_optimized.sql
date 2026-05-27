PROVENANCE OF (
    select *
    from (
            select ps_partkey,
                sum(ps_supplycost * ps_availqty) as value
            from partsupp USE PROVENANCE (ps_partkey),
                supplier USE PROVENANCE (s_suppkey),
                nation USE PROVENANCE (n_nationkey)
            where ps_suppkey = s_suppkey
                and s_nationkey = n_nationkey
                and n_name = 'GERMANY'
            group by ps_partkey
        )
    where value > (
            select sum(ps_supplycost * ps_availqty) * 0.0001000000
            from partsupp USE PROVENANCE (ps_partkey),
                supplier USE PROVENANCE (s_suppkey),
                nation USE PROVENANCE (n_nationkey)
            where ps_suppkey = s_suppkey
                and s_nationkey = n_nationkey
                and n_name = 'GERMANY'
        )
    order by value desc
);