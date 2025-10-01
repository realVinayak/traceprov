SELECT prov_skew__1__0__num__5000000_id FROM (PROVENANCE OF (select sum(val), z from skew_1_0_num_5000000 USE PROVENANCE (id) group by z)) F;
