select
    fast_bit_or(set_bit(0::bit(4000), binary_search_array_pos(%l_orderkey%, col_1::int) - 1, 1))::bit(4000) as a,
    fast_bit_or(set_bit(0::bit(4000), binary_search_array_pos(%p_partkey%, col_3::int) - 1, 1))::bit(4000) as b
from traceprov_relation_infer_1
