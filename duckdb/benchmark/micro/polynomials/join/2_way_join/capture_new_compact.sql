select
    1,
    traceprov_log_entry_2(
        1,
        t1.rowid :: int,
        t2.rowid :: int
    ) AS tp_table_1
from
    skew_1_0_num_NUM as t1
    JOIN polynomial_table_control_0 as t2 ON t1.z = t2.id
order by
    t1.rowid