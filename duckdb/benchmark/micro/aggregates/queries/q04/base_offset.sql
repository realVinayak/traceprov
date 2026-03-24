select * from (select avg(val),
    z
from skew_1_0_num_ROW_COUNT
group by z
having avg(val) > 50) LIMIT 1 OFFSET __TP_OFFSET__