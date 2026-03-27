-- using default substitutions
-- $ID$
-- TPC-H/TPC-R Parts/Supplier Relationship Query (Q16)
-- Functional Query Definition
-- Approved February 1998


-- optimizations:
-- * skip read|writeFilter()


select
    writeOrderBy(1, _t.tuid, row_number() over _order) as tuid,
    p_brand as p_brand,
    p_type as p_type,
    p_size as p_size,
    supplier_cnt as supplier_cnt
from
    (
        select
            writeAggregation(2, array_agg(_t.tuid)) as tuid,
            p_brand as p_brand,
            p_type as p_type,
            p_size as p_size,
            count(ps_suppkey) as supplier_cnt
        from
            (
                select distinct on (p_brand, p_type, p_size, ps_suppkey)
                    writeJoin(3, ps.tuid, p.tuid) as tuid,
                    p_brand as p_brand,
                    p_type as p_type,
                    p_size as p_size,
                    ps_suppkey as ps_suppkey
                from
                	partsupp_1 ps,
                	part_1 p
                where
                	p_partkey = ps_partkey
                	and p_brand <> 'Brand#45'
                	and p_type not like 'MEDIUM POLISHED%'
                	and p_size in (49, 14, 23, 45, 19, 3, 36, 9)
                	and ps_suppkey not in (
                		select
                            s_suppkey
                        from
                            (
                                select
                                    writeFilter(5, s.tuid) as tuid,
                        			s_suppkey as s_suppkey
                        		from
                        			supplier_1 s
                        		where
                        			s_comment like '%Customer%Complaints%'
                            ) as _t
                	)
            ) as _t
        group by
        	p_brand,
        	p_type,
        	p_size
    ) as _t
window
    _order as (
        order by
            supplier_cnt desc,
            p_brand,
            p_type,
            p_size
    )
order by
    row_number() over _order