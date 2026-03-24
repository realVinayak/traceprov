select avg(val),
    z
from skew_1_0_num_ROW_COUNT
where rowid in (
        select column_1
        from traceprov_lineage_1
    )
group by z;