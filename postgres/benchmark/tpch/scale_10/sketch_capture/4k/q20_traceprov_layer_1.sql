select
    fast_bit_or(set_bit(0::bit(4000), binary_search_array_pos(%l_orderkey%, col_6::int) - 1, 1)::bit(4000))::bit(4000) as a
from traceprov_relation_infer_1
