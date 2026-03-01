-- using default substitutions
select ps_partkey,
    sum(ps_supplycost * ps_availqty) as value
from partsupp,
    supplier,
    nation
where ps_suppkey = s_suppkey
    and s_nationkey = n_nationkey
    and (ps_partkey, ps_suppkey, s_suppkey, n_nationkey) in (
        select column_1,
            column_2,
            column_3,
            column_4
        FROM traceprov_lineage_3
    )
group by ps_partkey
having sum(ps_supplycost * ps_availqty) > (
        select sum(ps_supplycost * ps_availqty) * 0.0001000000
        from partsupp,
            supplier,
            nation
        where ps_suppkey = s_suppkey
            and s_nationkey = n_nationkey
            and (ps_partkey, ps_suppkey, s_suppkey, n_nationkey) in (
                select column_1,
                    column_2,
                    column_3,
                    column_4
                FROM traceprov_lineage_1
            )
    )
order by value desc;