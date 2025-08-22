select count(*) from (
    select * from (select * from (
        select count(*) AS cnt, block from crimes
        where id in (%s)
        group by block
    ) f1) f2 where cnt > 10000
) f3;

-- select count(*) from (
--     select * from (select * from (
--         select count(*) AS cnt, block from crimes
--         group by block
--     ) f1) f2 where cnt > 10000
-- ) f3;
