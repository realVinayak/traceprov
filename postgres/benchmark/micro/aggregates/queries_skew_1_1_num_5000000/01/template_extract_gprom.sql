SELECT prov_skew__1__1__num__5000000_id FROM (PROVENANCE OF (select count(val), z from skew_1_1_num_5000000 USE PROVENANCE (id) group by z)) F;
