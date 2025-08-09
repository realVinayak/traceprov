-- select count(*) from (select * from (select * from (select count(*) AS cnt, block from crimes group by block) f1) f2 where cnt > 10000) f3;

select count(*) from (
    select *, mark_later(comb_map) from (
        select * from (select count(*) AS cnt, block, agg_map(id) as comb_map from crimes group by block) f1
        ) f2 where cnt > 10000
    ) f3;