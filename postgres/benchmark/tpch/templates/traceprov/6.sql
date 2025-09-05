-- $ID$
-- TPC-H/TPC-R Forecasting Revenue Change Query (Q6)
-- Functional Query Definition
-- Approved February 1998
:x
:o
SELECT *, mark_later(mapped_agg) FROM (
    select
        sum(l_extendedprice * l_discount) as revenue,
        traceprov_agg_key_parallel(1, l_orderkey, l_linenumber) as mapped_agg
    from
        lineitem
    where
        l_shipdate >= date ':1'
        and l_shipdate < date ':1' + interval '1' year
        and l_discount between :2 - 0.01 and :2 + 0.01
        and l_quantity < :3
) f;
