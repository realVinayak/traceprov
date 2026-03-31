select
    fast_bit_or(set_bit(0::bit(10000), binary_search_array_pos(%o_orderkey%, col_4::int) - 1, 1)::bit(10000))::bit(10000) as b,
    fast_bit_or(set_bit(0::bit(10000), binary_search_array_pos(%c_custkey%, col_5::int) - 1, 1)::bit(10000))::bit(10000) as c
from traceprov_relation_infer_1
