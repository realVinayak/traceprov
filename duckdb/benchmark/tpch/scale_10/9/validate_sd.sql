-- using default substitutions
select nation,
    o_year,
    sum(amount) as sum_profit
from (
        select n_name as nation,
            extract(
                year
                from o_orderdate
            ) as o_year,
            l_extendedprice * (1 - l_discount) - ps_supplycost * l_quantity as amount
        from (select * from part where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 1)) part,
            (select * from supplier where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 4)) supplier,
            (select * from lineitem where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 0)) lineitem,
            (select * from partsupp where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 10)) partsupp,
            (select * from orders where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 8)) orders,
            (select * from nation where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 5)) nation
        where s_suppkey = l_suppkey
            and ps_suppkey = l_suppkey
            and ps_partkey = l_partkey
            and p_partkey = l_partkey
            and o_orderkey = l_orderkey
            and s_nationkey = n_nationkey
    ) as profit
group by nation,
    o_year
order by nation,
    o_year desc;
