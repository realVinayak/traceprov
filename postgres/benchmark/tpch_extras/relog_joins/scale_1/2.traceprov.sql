select count(*), o_custkey, traceprov_agg_from_ptr_dup_aware(1, 4, mapped_agg) as mapped_agg_later from orders join 
    (
        select 
        l_orderkey, 
        count(*), traceprov_agg_key_parallel(1, l_orderkey, l_linenumber) as mapped_agg
        from lineitem group by l_orderkey
    ) f
    on f.l_orderkey = o_orderkey
    group by o_custkey;

select count(*), o_custkey, traceprov_agg_key_parallel(4, mapped_agg) as mapped_agg_later from orders join 
    (
        select 
        l_orderkey, 
        count(*), traceprov_agg_key_parallel(1, l_orderkey, l_linenumber) as mapped_agg
        from lineitem group by l_orderkey
    ) f
    on f.l_orderkey = o_orderkey
    group by o_custkey;

select count(*), o_custkey, traceprov_agg_from_ptr(1, 4, mapped_agg) as mapped_agg_later from orders join 
    (
        select 
        l_orderkey, 
        count(*), traceprov_agg_key_parallel(1, l_orderkey, l_linenumber) as mapped_agg
        from lineitem group by l_orderkey
    ) f
    on f.l_orderkey = o_orderkey
    group by o_custkey;

select count(*), o_custkey, sum(mapped_agg) as mapped_agg_later from orders join 
    (
        select 
        l_orderkey, 
        count(*), traceprov_agg_key_parallel(1, l_orderkey, l_linenumber) as mapped_agg
        from lineitem group by l_orderkey
    ) f
    on f.l_orderkey = o_orderkey
    group by o_custkey;