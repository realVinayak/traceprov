select * from (select sum(val), z from skew_1_0_num_ROW_COUNT group by z
) LIMIT 1 OFFSET __TP_OFFSET__