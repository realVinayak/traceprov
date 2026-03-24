select avg(val),
    z
from skew_1_0_num_ROW_COUNT
where rowid in (
        select "iid"
        from "LAYER_1_SD_%OUT_ID%"
    )
group by z