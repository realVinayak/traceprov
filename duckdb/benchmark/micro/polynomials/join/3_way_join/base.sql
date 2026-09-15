select
    1
from
    skew_1_0_num_NUM as t1
    join (
        select
            t3.id
        from
            polynomial_table_control_1 as t3
            join polynomial_table_control_0 as t2 on t3.id = t2.id
    ) as t3 on t1.z = t3.id