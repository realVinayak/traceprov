SELECT prov_skew__1__1__num__10000000_id FROM (PROVENANCE OF (select count(val), z from skew_1_1_num_10000000 USE PROVENANCE (id) group by z)) F;
