-- select count(*) from (select * from (select * from (select count(*) AS cnt, block from crimes group by block) f1) f2 where cnt > 10000) f3;

explain select count(*) from (
    select *, mark_later(comb_map) from (
        select * from (select count(*) AS cnt, block, agg_map_parallel(id) as comb_map from crimes group by block) f1
        ) f2 where cnt > 10000
    ) f3;