-- using default substitutions
select o_year,
    sum(
        case
            when nation = 'BRAZIL' then volume
            else 0
        end
    ) / sum(volume) as mkt_share
from (
        select extract(
                year
                from o_orderdate
            ) as o_year,
            l_extendedprice * (1 - l_discount) as volume,
            n2.n_name as nation
        from (select * from part where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 1)) part,
            (select * from supplier where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 14)) supplier,
            (select * from lineitem where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 0)) lineitem,
            (select * from orders where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 4)) orders,
            (select * from customer where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 6)) customer,
            (select * from nation where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 7)) n1,
            (select * from nation where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 15)) n2,
            (select * from region where rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 8)) region
        where p_partkey = l_partkey
            and s_suppkey = l_suppkey
            and l_orderkey = o_orderkey
            and o_custkey = c_custkey
            and c_nationkey = n1.n_nationkey
            and n1.n_regionkey = r_regionkey
            and s_nationkey = n2.n_nationkey
    ) as all_nations
group by o_year
order by o_year;