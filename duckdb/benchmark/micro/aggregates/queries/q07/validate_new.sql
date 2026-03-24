select count(*) as c,
    z
from skew_1_0_num_ROW_COUNT
where rowid in (
        SELECT column_1
        from traceprov_lineage_1
    )
group by z
ORDER BY c desc