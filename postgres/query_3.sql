
-- select 
--     district, community_area, ward, beat, count(beat) as crime_count 
--     from crimes 
--     group by district, community_area, ward, beat 
--     having count(id) > 1000
--     order by crime_count;

-- select *, mark_later(mapped_agg) 
--     from (
--         select district, community_area, ward, beat, count(beat)  as crime_count, agg_map_parallel(id) as mapped_agg
--         from crimes group by district, community_area, ward, beat having count(id) > 1000
--     ) as f order by crime_count;

select 
    district, community_area, ward, beat, 
    count(beat) as crime_count
    from crimes
    where id in (%s)
group by district, community_area, ward, beat having count(id) > 1000
order by crime_count;