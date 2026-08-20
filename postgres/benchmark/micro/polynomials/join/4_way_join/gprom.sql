select
    t1.id :: text || ' ⊗ ' || t2.id :: text || ' ⊗ ' || t3.id :: text || ' ⊗ ' || t4.id :: text
from
    skew_1_0_num_NUM as t1
    JOIN polynomial_table_control_0 as t2 ON t1.z = t2.id
    JOIN polynomial_table_control_1 as t3 ON t2.id = t3.id
    JOIN polynomial_table_control_2 as t4 ON t3.id = t4.id;