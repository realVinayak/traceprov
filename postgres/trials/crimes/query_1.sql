select * from (
    select count(*) AS c, district, ward, block, community_area, beat 
    from crimes
    where id in (%s)
    group by district, ward, block, community_area, beat
    ) f order by c desc limit 5;


-- RAW -- 
-- select * from (
--     select count(*) AS c, district, ward, block, community_area, beat 
--     from crimes
--     group by district, ward, block, community_area, beat
--     ) f order by c desc limit 5;