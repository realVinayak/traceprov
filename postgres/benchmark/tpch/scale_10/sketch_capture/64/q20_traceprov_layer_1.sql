select
    fast_bit_or(set_bit(0::bit(64), binary_search_array_pos(%l_orderkey%, col_6::int) - 1, 1)::bit(64))::bit(64) as a
from traceprov_relation_infer_1
