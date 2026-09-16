select
    sr_formula(provenance(), 'factor_mapping')
from
    (
        select
            distinct 1
        from
            polynomial_table_0
            join polynomial_table_1 on polynomial_table_0.b = polynomial_table_1.b
            join polynomial_table_2 on polynomial_table_1.b = polynomial_table_2.b
        where
            polynomial_table_0.id <= NUM
            and polynomial_table_1.id <= NUM
            and polynomial_table_2.id <= NUM
    );