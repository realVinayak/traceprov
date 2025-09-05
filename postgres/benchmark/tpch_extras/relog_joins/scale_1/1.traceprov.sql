select count(*), l_linenumber, traceprov_agg_from_ptr_dup_aware(1, 4, mapped_agg) as mapped_agg_later from (select 
        l_orderkey as l_ok_1, 
        count(*), 
        traceprov_agg_key_parallel(1, l_orderkey, l_linenumber) as mapped_agg
    from lineitem 
    group by l_orderkey
    ) f 
    join lineitem on l_ok_1 = l_orderkey
    group by l_linenumber;

select count(*), l_linenumber, traceprov_agg_key_parallel(4, mapped_agg) as mapped_agg_later from (select 
        l_orderkey as l_ok_1, 
        count(*), 
        traceprov_agg_key_parallel(1, l_orderkey, l_linenumber) as mapped_agg
    from lineitem 
    group by l_orderkey
    ) f 
    join lineitem on l_ok_1 = l_orderkey
    group by l_linenumber;
