-- using default substitutions
select
    sum(l_extendedprice * l_discount) as revenue
from
    lineitem
where
    lineitem.rowid in (
        select
            column_0
        FROM
            LAYER_1
    );