select count(*) as c,
    z
from skew_1_0_num_ROW_COUNT
where rowid in (
        select column_1
        from "LAYER_1_%OUT_ID%"
    )
group by z
ORDER BY c