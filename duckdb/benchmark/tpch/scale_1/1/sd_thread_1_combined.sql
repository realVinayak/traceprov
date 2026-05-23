select a.out_index as column_0,
    c.in_index as column_1
from lineage_view(QID, 7) a
    JOIN lineage_view(QID, 4) b on a.in_index = b.out_index
    JOIN lineage_view(QID, 0) c on c.out_index = b.in_index;