select
    sr_formula(provenance(), 'factor_mapping')
from
    (
        select
            distinct 1
        from
            polynomial_table_0
        where
            id <= NUM
    )
    cross JOIN (
        select
            distinct 1
        from
            polynomial_table_1
        where
            id <= NUM
    )
    cross JOIN (
        select
            distinct 1
        from
            polynomial_table_2
        where
            id <= NUM
    )