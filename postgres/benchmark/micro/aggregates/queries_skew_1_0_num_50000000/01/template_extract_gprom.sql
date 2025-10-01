SELECT prov_skew__1__0__num__50000000_id FROM (PROVENANCE OF (select count(val), z from skew_1_0_num_50000000 USE PROVENANCE (id) group by z)) F;
