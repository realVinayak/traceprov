SELECT prov_part_p__partkey,
    prov_supplier_s__suppkey,
    prov_lineitem_l__orderkey,
    prov_partsupp_ps__partkey,
    prov_orders_o__orderkey,
    prov_nation_n__nationkey
FROM (
        PROVENANCE OF (
            select nation,
                o_year,
                sum(amount) as sum_profit
            from (
                    select n_name as nation,
                        date_part('YEAR', o_orderdate::date) as o_year,
                        l_extendedprice * (1 - l_discount) - ps_supplycost * l_quantity as amount
                    from part USE PROVENANCE (p_partkey),
                        supplier USE PROVENANCE (s_suppkey),
                        lineitem USE PROVENANCE (l_orderkey),
                        partsupp USE PROVENANCE (ps_partkey),
                        orders USE PROVENANCE (o_orderkey),
                        nation USE PROVENANCE (n_nationkey)
                    where s_suppkey = l_suppkey
                        and ps_suppkey = l_suppkey
                        and ps_partkey = l_partkey
                        and p_partkey = l_partkey
                        and o_orderkey = l_orderkey
                        and s_nationkey = n_nationkey
                        and p_name like '%green%'
                ) as profit
            group by nation,
                o_year
            order by nation,
                o_year desc
        )
    );
-- -- using default substitutions
-- select
-- 	nation,
-- 	o_year,
-- 	sum(amount) as sum_profit
-- from
-- 	(
-- 		select
-- 			n_name as nation,
-- 			extract(year from o_orderdate) as o_year,
-- 			l_extendedprice * (1 - l_discount) - ps_supplycost * l_quantity as amount
-- 		from
-- 			part,
-- 			supplier,
-- 			lineitem,
-- 			partsupp,
-- 			orders,
-- 			nation
-- 		where
-- 			s_suppkey = l_suppkey
-- 			and ps_suppkey = l_suppkey
-- 			and ps_partkey = l_partkey
-- 			and p_partkey = l_partkey
-- 			and o_orderkey = l_orderkey
-- 			and s_nationkey = n_nationkey
-- 			and p_name like '%green%'
-- 	) as profit
-- group by
-- 	nation,
-- 	o_year
-- order by
-- 	nation,
-- 	o_year desc;