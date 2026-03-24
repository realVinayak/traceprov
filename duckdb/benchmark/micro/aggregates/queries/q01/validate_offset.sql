select count(val),
    z
from skew_1_0_num_ROW_COUNT
where id in (
        select column_1
        from "LAYER_1_%OUT_ID%"
    )
group by z;