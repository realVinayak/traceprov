-- using default substitutions
select s_name,
    s_address
from supplier,
    nation
where s_suppkey in (
        select ps_suppkey
        from partsupp
        where ps_partkey in (
                select p_partkey
                from part
                where part.rowid in (
                        select column_0
                        from traceprov_lineage_1
                    )
            )
            and ps_availqty > (
                select 0.5 * sum(l_quantity)
                from lineitem
                where l_partkey = ps_partkey
                    and l_suppkey = ps_suppkey
                    and lineitem.rowid in (
                        select column_1_1
                        from traceprov_lineage_2
                    )
            )
            and partsupp.rowid in (
                select column_0
                from traceprov_lineage_4
            )
    )
    and (supplier.rowid, nation.rowid) in (
        select column_0,
            column_1
        from traceprov_lineage_5
    )
    and s_nationkey = n_nationkey
order by s_name;