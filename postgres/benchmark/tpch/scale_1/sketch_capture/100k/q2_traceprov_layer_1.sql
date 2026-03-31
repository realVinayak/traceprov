select
    fast_bit_or(set_bit(0::bit(100000), binary_search_array_pos(%p_partkey%, col_0::int) - 1, 1)::bit(100000))::bit(100000) as prov_part_p_partkey1,
    fast_bit_or(set_bit(0::bit(100000), binary_search_array_pos(%s_suppkey%, col_1::int) - 1, 1)::bit(100000))::bit(100000) as prov_supplier_s_suppkey1,
    fast_bit_or(set_bit(0::bit(100000), binary_search_array_pos(%ps_partkey%, col_2::int) - 1, 1)::bit(100000))::bit(100000) as prov_partsupp_ps_partkey1,
    fast_bit_or(set_bit(0::bit(100000), binary_search_array_pos(%ps_partkey%, col_7::int) - 1, 1)::bit(100000))::bit(100000) as prov_partsupp_ps_partkey2,
    fast_bit_or(set_bit(0::bit(100000), binary_search_array_pos(%s_suppkey%, col_10::int) - 1, 1)::bit(100000))::bit(100000) as prov_supplier_s_suppkey2
from traceprov_relation_infer_1
