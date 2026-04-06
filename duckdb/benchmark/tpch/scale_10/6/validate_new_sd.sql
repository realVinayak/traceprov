-- using default substitutions
select
    sum(l_extendedprice * l_discount) as revenue
from
    lineitem
where
    lineitem.rowid in (
        select
            opid_2_lineitem
        FROM
            LAYER_1
    );