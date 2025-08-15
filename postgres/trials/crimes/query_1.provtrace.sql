select *, mark_later(mapped_agg) from (
    select 
        count(*) AS c, 
        district, ward, block, 
        community_area, beat, 
        agg_map_parallel(id) as mapped_agg 
    from crimes
    group by district, ward, block, community_area, beat
    ) f order by c desc limit 5;