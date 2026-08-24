select * from (select
    distinct 1
from
    polynomial_table_0
    cross join polynomial_table_1
    cross join polynomial_table_2
where
    polynomial_table_0.id <= NUM
    and polynomial_table_1.id <= NUM
    and polynomial_table_2.id <= NUM) LIMIT 1 OFFSET __TP_OFFSET__