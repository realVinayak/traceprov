select
    t1.rowid :: text || ' ⊗ ' || t2.rowid :: text || ' ⊗ ' || t3.rowid :: text || ' ⊗ ' || t4.rowid :: text
from
    skew_1_0_num_NUM as t1
    JOIN polynomial_table_control_0 as t2 ON t1.z = t2.id
    JOIN polynomial_table_control_1 as t3 ON t2.id = t3.id
    JOIN polynomial_table_control_2 as t4 ON t3.id = t4.id
order by
    t1.rowid