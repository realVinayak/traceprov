-- using default substitutions
select
    100.00 * sum(
        case
            when p_type like 'PROMO%' then l_extendedprice * (1 - l_discount)
            else 0
        end
    ) / sum(l_extendedprice * (1 - l_discount)) as promo_revenue
from
    lineitem,
    part
where
    (l_orderkey, l_linenumber, p_partkey) in (
        select
            column_1,
            column_2,
            column_3
        from
            'LAYER_1'
    )