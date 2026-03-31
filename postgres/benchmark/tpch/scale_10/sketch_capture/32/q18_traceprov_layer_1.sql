select
    fast_bit_or(set_bit(0::bit(400), binary_search_array_pos(%c_custkey%, col_1::int) - 1, 1)::bit(100000))::bit(400) as a,
    fast_bit_or(set_bit(0::bit(400), binary_search_array_pos(%o_orderkey%, col_2::int) - 1, 1)::bit(100000))::bit(400) as b,
    fast_bit_or(set_bit(0::bit(400), binary_search_array_pos(%l_orderkey%, col_3::int) - 1, 1)::bit(100000))::bit(400) as c
from traceprov_relation_infer_2;