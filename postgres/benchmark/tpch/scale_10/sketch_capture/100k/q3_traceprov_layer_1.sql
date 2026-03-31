select
    fast_bit_or(set_bit(0::bit(100000), binary_search_array_pos(%l_orderkey%, col_3::int) - 1, 1)::bit(100000))::bit(100000) as a,
    fast_bit_or(set_bit(0::bit(100000), binary_search_array_pos(%o_orderkey%, col_2::int) - 1, 1)::bit(100000))::bit(100000) as c,
    fast_bit_or(set_bit(0::bit(100000), binary_search_array_pos(%c_custkey%, col_1::int) - 1, 1)::bit(100000))::bit(100000) as b
from traceprov_relation_infer_1
