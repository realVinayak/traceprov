-- using default substitutions
select sum(l_extendedprice * l_discount) as revenue
from lineitem
where (l_orderkey, l_linenumber) in (
        select column_1,
            column_2
        FROM traceprov_lineage_1
    );