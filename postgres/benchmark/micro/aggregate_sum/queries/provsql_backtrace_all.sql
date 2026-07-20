select provall from (
    select 
        sum(val),
        z,
        sr_which(provenance(), 'skew_SKEW_VALUE_num_ROW_COUNT_mapping') as provall
    from skew_SKEW_VALUE_num_ROW_COUNT group by z
);