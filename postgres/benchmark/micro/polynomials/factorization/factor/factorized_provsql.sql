select
    sr_formula(provenance(), 'factor_mapping')
from
    (
        select
            distinct b
        from
            polynomial_table_0
        where
            id <= NUM
    ) as t1
    JOIN (
        select
            distinct b
        from
            polynomial_table_1
        where
            id <= NUM
    ) as t2 on t1.b = t2.b
    JOIN (
        select
            distinct b
        from
            polynomial_table_2
        where
            id <= NUM
    ) as t3 on t2.b = t3.b