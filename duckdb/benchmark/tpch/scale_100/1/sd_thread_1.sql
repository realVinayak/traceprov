select a.out_index,
    c.in_index
from lineage_view(1, 7) a
    JOIN lineage_view(1, 4) b on a.in_index = b.out_index
    JOIN lineage_view(1, 0) c on c.out_index = b.in_index;