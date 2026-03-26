-- using default substitutions
-- SELECT
--     tp_table_0.c_count,
--     tp_table_0.custdist,
--     tp_table_0.mapped_agg,
--     traceprov_log_entry (3, tp_table_0.mapped_agg) AS tp_table_1
-- FROM
--     (
--         SELECT
--             c_orders.c_count,
--             count(*) AS custdist,
--             traceprov_agg_key_parallel_offset (2, c_orders.mapped_agg) AS mapped_agg
--         FROM
--             (
--                 SELECT
--                     customer.c_custkey,
--                     count(orders.o_orderkey) AS count,
--                     traceprov_agg_key_parallel_offset (
--                         1,
--                         (customer.c_custkey)::bigint,
--                         (orders.o_orderkey)::bigint
--                     ) AS mapped_agg
--                 FROM
--                     (
--                         customer
--                         LEFT JOIN orders ON (
--                             (
--                                 (customer.c_custkey = orders.o_custkey)
--                                 AND (
--                                     (orders.o_comment)::text !~~ '%special%requests%'::text
--                                 )
--                             )
--                         )
--                     )
--                 GROUP BY
--                     customer.c_custkey
--             ) c_orders (c_custkey, c_count, mapped_agg)
--         GROUP BY
--             c_orders.c_count
--         ORDER BY
--             (count(*)) DESC,
--             c_orders.c_count DESC
--     ) tp_table_0;
select c_count,
    count(*) as custdist
from (
        select c_custkey,
            count(o_orderkey)
        from customer
            left outer join orders on c_custkey = o_custkey
            and o_comment not like '%special%requests%'
        where EXISTS (
                select *
                from traceprov_relation_infer_1_mat
                where col_2 is not distinct
                from c_custkey
                    and col_3 is not distinct
                from o_orderkey
            )
        group by c_custkey
    ) as c_orders (c_custkey, c_count)
group by c_count
order by custdist desc,
    c_count desc;