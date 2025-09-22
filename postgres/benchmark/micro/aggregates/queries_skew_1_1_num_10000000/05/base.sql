select 
    min_value, count(f.z) 
    from (
            select min(val) as min_value, 
                z from skew_1_1_num_10000000 
                group by z
        ) f 
    group by min_value 
having (count(f.z)) > 900;