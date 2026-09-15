select
    1
from
    skew_1_0_num_NUM as t1
    join (
        select
            t2.id
        from
            polynomial_table_control_0 as t2
            join (
                select
                    t3.id
                from
                    polynomial_table_control_2 as t4
                    join polynomial_table_control_1 as t3 on t3.id = t4.id
            ) as t3 on t2.id = t3.id
    ) as t2 on t1.z = t2.id
order by
    t1.rowid