1. DuckDB SF=10 GProM offset: 

```
duckdb/benchmark/tpch/scale_10/results/macos_gprom_offset_reduced_optimized-y__threads-1_2026_07_10_14_01_55/
```

2. DuckDB SF=10 GProM all:

```
duckdb/benchmark/tpch/scale_10/results/macos_gprom_optimized-y__threads-1_2026_07_10_16_18_37/
```

3. DuckDB SF=1 GProM all:

```
duckdb/benchmark/tpch/scale_1/results/macos_gprom_optimized-y__threads-1_2026_07_10_17_17_08/
```

4. DuckDB SF=1 GProM offset:
```
duckdb/benchmark/tpch/scale_1/results/macos_gprom_offset_optimized-y__threads-1_2026_07_11_02_19_10/
```

1. Postgres SF=1 GProM all:

```
postgres/benchmark/tpch/scale_1/results/macos_gprom_all_gprom_params_default_restricted_2026_07_11_04_33_01/
```

2. Postgres SF=1 GProM offset:

```
postgres/benchmark/tpch/scale_1/results/macos_gprom_offset_gprom_params_default_restricted_keys_2026_07_12_01_03_44/
```

3. Postgres SF=10 GProM all:

```
postgres/benchmark/tpch/scale_10/results/macos_gprom_all_gprom_params_default_restricted_2026_07_12_11_08_52/
```

4.  Postgres SF=10 GProM offset # (offset - Part 1 - Exclude 11, 16, 18):

```
postgres/benchmark/tpch/scale_10/results/macos_gprom_offset_e_11_16_gprom_params_default_restricted_keys_2026_07_13_23_14_03/
```

5. Postgres SF=10 GProM offset # (offset - Part 2 - 11 and 16, sampled):

```
postgres/benchmark/tpch/scale_10/results/macos_gprom_offset_18_gprom_params_default_restricted_keys_2026_07_14_21_36_51/
```

6. Postgres SF=1 Muller all:
```
postgres/benchmark/tpch/scale_1/results/muller_macos_rows_all_2026_07_14_23_40_47/
```

7. Postgres SF=1 Muller offset:
```
postgres/benchmark/tpch/scale_1/results/muller_macos_rows_all_2026_07_15_04_39_19/
```

8. Postgres SF=10 Muller all:
```
postgres/benchmark/tpch/scale_10/results/muller_macos_rows_all_2026_07_15_23_27_02/
```

9. Postgres SF=10 Muller offset:
```
postgres/benchmark/tpch/scale_10/results/muller_macos_rows_single_2026_07_15_22_13_15/
```

######################## Using original 4, 18, 20
10. DuckDB SF=1 GProM all # (q4, 18, 20, 21):
```
duckdb/benchmark/tpch/scale_1/results/macos_gprom_4_18_20_21_optimized-y__threads-1_2026_07_17_05_51_53/
```

11. DuckDB SF=1 GProM offset # (q4, 18, 20, 21):
```
duckdb/benchmark/tpch/scale_1/results/macos_gprom_offset_4_18_20_21_optimized-y__threads-1_2026_07_17_07_32_19/
```

12. DuckDB SF=10 GProM all # (q4, 18, 20, 21):
```
duckdb/benchmark/tpch/scale_10/results/macos_gprom_4_18_20_21_optimized-y__threads-1_2026_07_17_07_56_31/
```

13. DuckDB SF=10 GProM offset # (q4, 18, 20, 21):
```
duckdb/benchmark/tpch/scale_10/results/macos_gprom_offset_reduced_4_18_20_21_optimized-y__threads-1_2026_07_17_11_30_00/
```

14. Postgres SF=10 GProM all # (q4, 18, 20, 21):
```
postgres/benchmark/tpch/scale_10/results/macos_gprom_all_18_20_21_gprom_params_default_restricted_2026_07_17_04_14_46/
postgres/benchmark/tpch/scale_10/results/macos_gprom_all_4_gprom_params_default_restricted_2026_07_16_16_24_53/
```

15. Postgres SF=10 GProM offset # (q4, 18, 20, 21):
```
postgres/benchmark/tpch/scale_10/results/macos_test_macos_gprom_offset_18_20_21_gprom_params_default_restricted_keys_2026_07_17_04_49_19/
postgres/benchmark/tpch/scale_10/results/macos_test_macos_gprom_offset_4_gprom_params_default_restricted_keys_2026_07_16_16_59_28/
```

16. Postgres SF=1 GProM all # (q4, 18, 20, 21):
```
postgres/benchmark/tpch/scale_1/results/macos_gprom_all_18_20_21_gprom_params_default_restricted_2026_07_16_21_32_36/
postgres/benchmark/tpch/scale_1/results/macos_gprom_all_4_gprom_params_default_restricted_2026_07_16_17_09_38/
```

17. Postgres SF=1 GProM offset # (q4, 18, 20, 21):
```
postgres/benchmark/tpch/scale_1/results/macos_gprom_offset_18_20_21_gprom_params_default_restricted_keys_2026_07_17_03_43_37/
postgres/benchmark/tpch/scale_1/results/macos_gprom_offset_4_gprom_params_default_restricted_keys_2026_07_16_17_14_31/
```


18. DuckDB SF=10 GProM all # (SmokedDuck Version) [SF=10] (all mode):
```
duckdb/benchmark/tpch/scale_10/results/macos_gprom_optimized-y__threads-10_2026_07_17_17_09_04/
duckdb/benchmark/tpch/scale_10/results/macos_gprom_optimized-y__threads-12_2026_07_17_17_16_19/
duckdb/benchmark/tpch/scale_10/results/macos_gprom_optimized-y__threads-2_2026_07_17_16_45_45/
duckdb/benchmark/tpch/scale_10/results/macos_gprom_optimized-y__threads-4_2026_07_17_16_54_37/
duckdb/benchmark/tpch/scale_10/results/macos_gprom_optimized-y__threads-8_2026_07_17_17_01_47/
```

19. DuckDB SF=10 GProM offset # (SmokedDuck Version) [SF=10] (offset mode):
```
duckdb/benchmark/tpch/scale_10/results/macos_gprom_offset_optimized-y__threads-10_2026_07_18_00_40_22/
duckdb/benchmark/tpch/scale_10/results/macos_gprom_offset_optimized-y__threads-12_2026_07_18_02_09_39/
duckdb/benchmark/tpch/scale_10/results/macos_gprom_offset_optimized-y__threads-2_2026_07_17_19_56_40/
duckdb/benchmark/tpch/scale_10/results/macos_gprom_offset_optimized-y__threads-4_2026_07_17_21_41_38/
duckdb/benchmark/tpch/scale_10/results/macos_gprom_offset_optimized-y__threads-8_2026_07_17_23_11_36/
```

20. NewDuckDB SF=10 GProM all # (Latest DuckDB version):
```
duckdb/benchmark/tpch/scale_10/results/macos_gprom_latest_duck_optimized-y__threads-10_2026_07_18_12_41_34/
duckdb/benchmark/tpch/scale_10/results/macos_gprom_latest_duck_optimized-y__threads-12_2026_07_18_13_52_04/
duckdb/benchmark/tpch/scale_10/results/macos_gprom_latest_duck_optimized-y__threads-1_2026_07_18_07_19_56/
duckdb/benchmark/tpch/scale_10/results/macos_gprom_latest_duck_optimized-y__threads-2_2026_07_18_08_57_07/
duckdb/benchmark/tpch/scale_10/results/macos_gprom_latest_duck_optimized-y__threads-4_2026_07_18_10_19_40/
duckdb/benchmark/tpch/scale_10/results/macos_gprom_latest_duck_optimized-y__threads-8_2026_07_18_11_31_38/
```

21. DuckDB SF=100 GProM all:
```
duckdb/benchmark/tpch/scale_100/results/macos_gprom_optimized-y__threads-1_2026_07_21_19_33_25/
```

22. DuckDB SF=100 SmokedDuck all:
```
duckdb/benchmark/tpch/scale_100/results/macos_sd_all_optimized-y__threads-1_2026_07_24_17_31_45
```

23. DuckDB SF=100 SmokedDuck offset:
```
duckdb/benchmark/tpch/scale_100/results/macos_sd_offset_optimized-y__threads-1_2026_07_25_09_39_27
```

24. DuckDB SF=100 TraceProv all:
```
duckdb/benchmark/tpch/scale_100/results/macos_tp_all_optimized-y__threads-1__compact-y__merge_chunks-y__table_stats-y_2026_07_25_14_58_15
```

25. DuckDB SF=100 TraceProv offset:
```
duckdb/benchmark/tpch/scale_100/results/macos_tp_offset_optimized-y__threads-1__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_26_16_30_06
```

26. DuckDB SF=1 SmokedDuck all:
```
duckdb/benchmark/tpch/scale_1/results/macos_sd_all_optimized-y__threads-10_2026_07_22_18_49_39/
duckdb/benchmark/tpch/scale_1/results/macos_sd_all_optimized-y__threads-12_2026_07_22_18_50_07/
duckdb/benchmark/tpch/scale_1/results/macos_sd_all_optimized-y__threads-1_2026_07_22_18_44_42/
duckdb/benchmark/tpch/scale_1/results/macos_sd_all_optimized-y__threads-2_2026_07_22_18_47_54/
duckdb/benchmark/tpch/scale_1/results/macos_sd_all_optimized-y__threads-4_2026_07_22_18_48_37/
duckdb/benchmark/tpch/scale_1/results/macos_sd_all_optimized-y__threads-8_2026_07_22_18_49_12/
```

26. DuckDB SF=1 SmokedDuck offset:
```
duckdb/benchmark/tpch/scale_1/results/macos_sd_offset_optimized-y__threads-10_2026_07_22_18_57_07/
duckdb/benchmark/tpch/scale_1/results/macos_sd_offset_optimized-y__threads-12_2026_07_22_18_57_25/
duckdb/benchmark/tpch/scale_1/results/macos_sd_offset_optimized-y__threads-1_2026_07_22_18_53_24/
duckdb/benchmark/tpch/scale_1/results/macos_sd_offset_optimized-y__threads-2_2026_07_22_18_56_07/
duckdb/benchmark/tpch/scale_1/results/macos_sd_offset_optimized-y__threads-4_2026_07_22_18_56_30/
duckdb/benchmark/tpch/scale_1/results/macos_sd_offset_optimized-y__threads-8_2026_07_22_18_56_48/
```

26. DuckDB SF=1 TraceProv all:
```
duckdb/benchmark/tpch/scale_1/results/macos_tp_all_optimized-y__threads-10__compact-y__merge_chunks-y__table_stats-y_2026_07_23_14_48_26/
duckdb/benchmark/tpch/scale_1/results/macos_tp_all_optimized-y__threads-12__compact-y__merge_chunks-y__table_stats-y_2026_07_23_14_49_21/
duckdb/benchmark/tpch/scale_1/results/macos_tp_all_optimized-y__threads-1__compact-y__merge_chunks-y__table_stats-y_2026_07_23_14_43_35/
duckdb/benchmark/tpch/scale_1/results/macos_tp_all_optimized-y__threads-2__compact-y__merge_chunks-y__table_stats-y_2026_07_23_14_45_21/
duckdb/benchmark/tpch/scale_1/results/macos_tp_all_optimized-y__threads-4__compact-y__merge_chunks-y__table_stats-y_2026_07_23_14_46_33/
duckdb/benchmark/tpch/scale_1/results/macos_tp_all_optimized-y__threads-8__compact-y__merge_chunks-y__table_stats-y_2026_07_23_14_47_29/
```

26. DuckDB SF=1 TraceProv offset:
```
duckdb/benchmark/tpch/scale_1/results/macos_tp_offset_optimized-y__threads-10__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_23_16_15_14/
duckdb/benchmark/tpch/scale_1/results/macos_tp_offset_optimized-y__threads-12__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_23_16_33_50/
duckdb/benchmark/tpch/scale_1/results/macos_tp_offset_optimized-y__threads-1__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_23_15_06_49/
duckdb/benchmark/tpch/scale_1/results/macos_tp_offset_optimized-y__threads-2__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_23_15_23_49/
duckdb/benchmark/tpch/scale_1/results/macos_tp_offset_optimized-y__threads-4__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_23_15_39_26/
duckdb/benchmark/tpch/scale_1/results/macos_tp_offset_optimized-y__threads-8__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_23_15_57_18/
```

###########################################################################

26. DuckDB SF=10 SmokedDuck all:
```
duckdb/benchmark/tpch/scale_10/results/macos_sd_all_optimized-y__threads-10_2026_07_22_22_05_21/
duckdb/benchmark/tpch/scale_10/results/macos_sd_all_optimized-y__threads-12_2026_07_22_22_25_41/
duckdb/benchmark/tpch/scale_10/results/macos_sd_all_optimized-y__threads-1_2026_07_22_20_09_21/
duckdb/benchmark/tpch/scale_10/results/macos_sd_all_optimized-y__threads-2_2026_07_22_20_55_14/
duckdb/benchmark/tpch/scale_10/results/macos_sd_all_optimized-y__threads-4_2026_07_22_21_21_11/
duckdb/benchmark/tpch/scale_10/results/macos_sd_all_optimized-y__threads-8_2026_07_22_21_47_47/
```

26. DuckDB SF=10 SmokedDuck offset:
```
duckdb/benchmark/tpch/scale_10/results/macos_sd_offset_optimized-y__threads-10_2026_07_23_03_16_23/
duckdb/benchmark/tpch/scale_10/results/macos_sd_offset_optimized-y__threads-12_2026_07_23_03_22_14/
duckdb/benchmark/tpch/scale_10/results/macos_sd_offset_optimized-y__threads-1_2026_07_23_00_44_01/
duckdb/benchmark/tpch/scale_10/results/macos_sd_offset_optimized-y__threads-2_2026_07_23_02_53_57/
duckdb/benchmark/tpch/scale_10/results/macos_sd_offset_optimized-y__threads-4_2026_07_23_03_03_16/
duckdb/benchmark/tpch/scale_10/results/macos_sd_offset_optimized-y__threads-8_2026_07_23_03_10_36/
```

26. DuckDB SF=10 TraceProv all:
```
duckdb/benchmark/tpch/scale_10/results/macos_tp_all_optimized-y__threads-10__compact-y__merge_chunks-y__table_stats-y_2026_07_23_20_29_40/
duckdb/benchmark/tpch/scale_10/results/macos_tp_all_optimized-y__threads-12__compact-y__merge_chunks-y__table_stats-y_2026_07_23_20_35_49/
duckdb/benchmark/tpch/scale_10/results/macos_tp_all_optimized-y__threads-1__compact-y__merge_chunks-y__table_stats-y_2026_07_23_19_46_10/
duckdb/benchmark/tpch/scale_10/results/macos_tp_all_optimized-y__threads-2__compact-y__merge_chunks-y__table_stats-y_2026_07_23_20_05_46/
duckdb/benchmark/tpch/scale_10/results/macos_tp_all_optimized-y__threads-4__compact-y__merge_chunks-y__table_stats-y_2026_07_23_20_16_47/
duckdb/benchmark/tpch/scale_10/results/macos_tp_all_optimized-y__threads-8__compact-y__merge_chunks-y__table_stats-y_2026_07_23_20_23_20/
```

26. DuckDB SF=10 TraceProv offset:
```
duckdb/benchmark/tpch/scale_10/results/macos_tp_offset_optimized-y__threads-10__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_24_07_20_42/
duckdb/benchmark/tpch/scale_10/results/macos_tp_offset_optimized-y__threads-12__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_24_09_07_30/
duckdb/benchmark/tpch/scale_10/results/macos_tp_offset_optimized-y__threads-1__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_23_23_30_34/
duckdb/benchmark/tpch/scale_10/results/macos_tp_offset_optimized-y__threads-2__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_24_02_17_19/
duckdb/benchmark/tpch/scale_10/results/macos_tp_offset_optimized-y__threads-4__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_24_04_07_55/
duckdb/benchmark/tpch/scale_10/results/macos_tp_offset_optimized-y__threads-8__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_24_05_41_19/
```

26. NewDuckDB SF=10 TraceProv all:
```
duckdb/benchmark/tpch/scale_10/results/macos_duckdb_latest_tp_all_optimized-y__threads-10__compact-y__merge_chunks-y__table_stats-y_2026_07_26_18_49_21/
duckdb/benchmark/tpch/scale_10/results/macos_duckdb_latest_tp_all_optimized-y__threads-12__compact-y__merge_chunks-y__table_stats-y_2026_07_26_18_53_14/
duckdb/benchmark/tpch/scale_10/results/macos_duckdb_latest_tp_all_optimized-y__threads-1__compact-y__merge_chunks-y__table_stats-y_2026_07_26_18_21_14/
duckdb/benchmark/tpch/scale_10/results/macos_duckdb_latest_tp_all_optimized-y__threads-2__compact-y__merge_chunks-y__table_stats-y_2026_07_26_18_33_53/
duckdb/benchmark/tpch/scale_10/results/macos_duckdb_latest_tp_all_optimized-y__threads-4__compact-y__merge_chunks-y__table_stats-y_2026_07_26_18_41_01/
duckdb/benchmark/tpch/scale_10/results/macos_duckdb_latest_tp_all_optimized-y__threads-8__compact-y__merge_chunks-y__table_stats-y_2026_07_26_18_45_17/
```

27. NewDuckDB SF=10 TraceProv offset:
```
duckdb/benchmark/tpch/scale_10/results/macos_duckdb_latest_tp_offset_optimized-y__threads-1__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_26_21_04_08/
duckdb/benchmark/tpch/scale_10/results/macos_duckdb_latest_tp_offset_optimized-y__threads-2__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_26_22_52_48/
duckdb/benchmark/tpch/scale_10/results/macos_duckdb_latest_tp_offset_optimized-y__threads-10__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_27_12_44_02
duckdb/benchmark/tpch/scale_10/results/macos_duckdb_latest_tp_offset_optimized-y__threads-12__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_27_14_23_39
duckdb/benchmark/tpch/scale_10/results/macos_duckdb_latest_tp_offset_optimized-y__threads-4__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_27_09_47_58
duckdb/benchmark/tpch/scale_10/results/macos_duckdb_latest_tp_offset_optimized-y__threads-8__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_27_11_09_45
```

28. Postgres SF=10 GProM all:
```
postgres/benchmark/tpch/scale_10/results/macos_gprom_all_procs_10_gprom_params_default_restricted_2026_07_20_01_42_15/
postgres/benchmark/tpch/scale_10/results/macos_gprom_all_procs_12_gprom_params_default_restricted_2026_07_20_05_11_28/
postgres/benchmark/tpch/scale_10/results/macos_gprom_all_procs_2_gprom_params_default_restricted_2026_07_19_15_13_25/
postgres/benchmark/tpch/scale_10/results/macos_gprom_all_procs_4_gprom_params_default_restricted_2026_07_19_18_45_11/
postgres/benchmark/tpch/scale_10/results/macos_gprom_all_procs_8_gprom_params_default_restricted_2026_07_19_22_14_03/
```

28. DuckDB SF=100 SmokedDuck all:
```
duckdb/benchmark/tpch/scale_100/results/macos_sd_all_optimized-y__threads-1_2026_07_24_17_31_45/
```

29. DuckDB SF=100 SmokedDuck offset:
```
duckdb/benchmark/tpch/scale_100/results/macos_sd_offset_optimized-y__threads-1_2026_08_07_14_07_40/
```

30. DuckDB SF=100 GProM all:
```
duckdb/benchmark/tpch/scale_100/results/macos_gprom_optimized-y__threads-1_2026_07_21_19_33_25/
```

31. DuckDB SF=100 GProM offset:
```
duckdb/benchmark/tpch/scale_100/results/macos_gprom_offset_optimized-y__threads-1_2026_08_10_00_31_29/
```

32. DuckDB SF=100 TraceProv all:
```
duckdb/benchmark/tpch/scale_100/results/macos_tp_all_optimized-y__threads-1__compact-y__merge_chunks-y__table_stats-y_2026_07_25_14_58_15/
```

33. DuckDB SF=100 TraceProv offset:
```
duckdb/benchmark/tpch/scale_100/results/macos_tp_offset_optimized-y__threads-1__compact-y__filter_pushdown-y__join_filter_rewrite-y__merge_chunks-y__partition_in_agg-y__table_stats-y_2026_07_26_16_30_06/
```

34. Postgres SF=10 ProvSQL all:
```
postgres/benchmark/tpch/scale_10/results/macos_provsql_2026_08_10_19_11_26/
```

35. Postgres SF=10 ProvSQL offset:
```
postgres/benchmark/tpch/scale_10/results/macos_provsql_offset_2026_08_10_23_33_29/
```

35. Postgres SF=1 ProvSQL all:
```
postgres/benchmark/tpch/scale_1/results/macos_provsql_2026_08_11_02_08_05/
```

36. Postgres SF=1 ProvSQL offset:
```
postgres/benchmark/tpch/scale_1/results/macos_provsql_offset_2026_08_11_02_37_07/
```

37. Postgres SF=10 TraceProv all:
```
postgres/benchmark/tpch/scale_10/results/macos_traceprov_all_proc_0_2026_07_27_20_29_43/
```

38. Postgres SF=10 TraceProv offset:
```
postgres/benchmark/tpch/scale_1/results/macos_provsql_offset_2026_08_11_02_37_07/
```

39. Postgres SF=10 TraceProv all:
```
postgres/benchmark/tpch/scale_1/results/macos_traceprov_all_proc_0_2026_08_13_03_32_49/
```

40. Postgres SF=10 TraceProv offset:
```
postgres/benchmark/tpch/scale_1/results/macos_traceprov_offset_proc_0_2026_08_13_03_56_48/
```