select
    1,
    traceprov_log_entry_3(
        1,
        t1.rowid :: int,
        t2.rowid :: int,
        t3.rowid :: int
    ) AS tp_table_1
from
    skew_1_0_num_NUM as t1
    JOIN polynomial_table_control_0 as t2 ON t1.z = t2.id
    JOIN polynomial_table_control_1 as t3 ON t2.id = t3.id;