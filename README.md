# TraceProv

Derives the provenance of the query using user-defined functions and aggregates, and outperforms SOTA database intrusive systems.

# Experimental Setup

## A. TPC-H

### Data:
We utilize `dbgen` TPC-H V3.0.1 to generate the TPC-H data, and the corresponding `qgen` to generate the queries. We utilize the official `check_answers` to verify that the dataset and the queries are correct.


### Queries:
The different baselines used in the paper rewrite the queries differently. The rewritten queries are organized as following. Here, `SCALE_FACTOR` is the scale factor (1, 10, 100) and `QUERY_NUM` is the query num (1..22).

_PostgreSQL_:

1. NoProv: `postgres/benchmark/tpch/scale_SCALE_FACTOR/params_default/QUERY_NUM/base.sql`. The same queries are used for DuckDB.
2. GProM:
    - P_all task: `postgres/benchmark/tpch/scale_SCALE_FACTOR/gprom_params_default_restricted/QUERY_NUM/gprom_window_heuristics.sql`
    - P_single task: `postgres/benchmark/tpch/scale_SCALE_FACTOR/gprom_params_default_restricted_keys/QUERY_NUM/gprom_window_heuristics.sql`
4. Müller (P_all and P_single task): The directory is `postgres/benchmark/tpch/scale_SCALE_FACTOR/muller_params_default/QUERY_NUM/`. Capture query is in `phase_1.sql` and Backtrace query is in `phase_2_row.sql`.
5. ProvSQL (P_all and P_single task): The directory is ``postgres/benchmark/tpch/scale_SCALE_FACTOR/params_default/QUERY_NUM/provsql_capture_backtrace.sql``.

_DuckDB_:

1. GProM
    - P_all task: `duckdb/benchmark/tpch/scale_SCALE_FACTOR/gprom_params_default_restricted_optimized/QUERY_NUM/gprom_window_heuristics.sql`
    - P_single task: `duckdb/benchmark/tpch/scale_SCALE_FACTOR/gprom_params_default_restricted_keys_optimized/QUERY_NUM/gprom_window_heuristics.sql`
2. SmokedDuck (P_all task): `duckdb/benchmark/tpch/scale_SCALE_FACTOR/QUERY_NUM/sd_thread_1_combined.sql`
3. TraceProv (DuckDB, P_all and P_single task): Capture: `duckdb/benchmark/tpch/scale_SCALE_FACTOR/QUERY_NUM/capture_new_compact.sql`. Note that for Backtrace, TraceProv generates the query on the fly and hence the query is unavailable.

### Execution

_PostgreSQL_: We run `postgres/benchmark/tpch/run.sh` in the scale factor directory i.e., postgres/benchmark/tpch/scale_SCALE_FACTOR directory.

_DuckDB_: We run `duckdb/benchmark/tpch/run.sh` in the scale factor directory i.e., postgres/benchmark/tpch/scale_SCALE_FACTOR directory.


## B. Provenance Polynomial Benchmark

### Data:
We use a Zipfian data generator adapted from [lineage_benchmark](https://github.com/haneensa/lineage_benchmark).

Run the following command to create PostgreSQL databases. Note that all the PostgreSQL data generation and benchmark scripts provide additional connection options as `--H <HOST> --P <PORT> -u <USER> -p <PASSWORD>`

```
cd postgres/benchmark/micro/polynomials/ && bash create_data.sh
```

Note that to create a copy for DuckDB, we simply use the PostgreSQL connector and copy all tables as below in DuckDB CLI:

```
INSTALL postgres;
LOAD postgres;

ATTACH 'host=<HOST> port=<PORT> dbname=polynomial_data user=<USER>' AS pg (TYPE postgres);
-- for each table <TABLE>:
create table <TABLE> as (select * from pg.<TABLE>)
```

### Queries (Factorization Study):

_PostgreSQL_:

1. NoProv: `postgres/benchmark/micro/polynomials/factorization/factor/factorized_base.sql`
2. GProM: `postgres/benchmark/micro/polynomials/factorization/non_factor/non_factorized_gprom.sql`
3. TraceProv
    - Backtrace:`postgres/benchmark/micro/polynomials/factorization/factor/factorized_traceprov.sql`
4. ProvSQL
    - Capture and Backtrace: `postgres/benchmark/micro/polynomials/factorization/factor/factorized_provsql.sql`

_DuckDB_:

1. NoProv: `duckdb/benchmark/micro/polynomials/factorization/factor/base.sql`
2. GProM: `duckdb/benchmark/micro/polynomials/factorization/non_factor/gprom.sql`
3. TraceProv:
    - Capture: `duckdb/benchmark/micro/polynomials/factorization/factor/capture_new_compact.sql`
    - Backtrace: `duckdb/benchmark/micro/polynomials/factorization/factor/traceprov.sql`
4. SmokedDuck:
    - Backtrace: `duckdb/benchmark/micro/polynomials/factorization/factor/traceprov.sql`

### Queries (Join Study):

Note that, here, `NUM` is 2, 3, or 4.

_PostgreSQL_:
1. NoProv: `postgres/benchmark/micro/polynomials/join/NUM_way_join/base.sql`
2. GProM: `postgres/benchmark/micro/polynomials/join/NUM_way_join/gprom.sql`
3. TraceProv:
    - Backtrace: `postgres/benchmark/micro/polynomials/join/NUM_way_join/traceprov.sql`
4. ProvSQL:
    - Capture and Backtrace: `postgres/benchmark/micro/polynomials/join/NUM_way_join/provsql.sql`

_DuckDB_:
1. NoProv: `duckdb/benchmark/micro/polynomials/join/NUM_way_join/base.sql`
2. GProM: `duckdb/benchmark/micro/polynomials/join/NUM_way_join/gprom.sql`
3. TraceProv: 
    - Capture: `duckdb/benchmark/micro/polynomials/join/NUM_way_join/capture_new_compact.sql`
    - Backtrace: `duckdb/benchmark/micro/polynomials/join/NUM_way_join/traceprov.sql`
4. SmokedDuck:
    - Backtrace: `duckdb/benchmark/micro/polynomials/join/NUM_way_join/sd_lineage_view_polynomial.sql`

### Execution

Run the following commands for the respective data to run the experiments:

_PostgreSQL_:
```
cd postgres/benchmark/micro/polynomials && bash run.sh
```

_DuckDB_:
```
cd duckdb/benchmark/micro/polynomials && bash run.sh
```


## C. Hash-Aggregate Benchmark 

### Data:
Run the following command to create PostgreSQL databases:

```
cd postgres/benchmark/micro/aggregates/ && bash create_databases.sh
```

To create DuckDB tables, we utilize the same copy technique as in "B. Provenance Polynomial" (i.e., using PostgreSQL connector to copy the tables).

### Queries:
_DuckDB_:
1. NoProv: `duckdb/benchmark/micro/aggregates/queries/q03/base.sql`
2. SmokedDuck:
    - Backtrace: `duckdb/benchmark/micro/aggregates/queries/q03/sd_thread_1_combined.sql`
3. TraceProv:
    - Capture: `duckdb/benchmark/micro/aggregates/queries/q03/capture_new_compact.sql`

### Execution:
To run DuckDB experiments, run the following command:

```
cd duckdb/benchmark/micro/aggregates/ && bash run.sh
```

## D. Log Cost Benchmark

### Data:
Run the following command to create the database for PostgreSQL:

```
cd postgres/benchmark/micro/log_cost/ && bash make_data.sh
```

To create DuckDB tables, we utilize the same copy technique as in "B. Provenance Polynomial" (i.e., using PostgreSQL connector to copy the tables).

### Execution:

To run PostgreSQL experiments, run the following command:
```
cd postgres/benchmark/micro/log_cost/ && bash run.sh
```

To run DuckDB experiments, run the following command:
```
cd duckdb/benchmark/micro/log_cost/ && bash run.sh
```

# Parallelism

For all the figures presented in the main paper, we use `DuckDB 0.10.3` and `PostgreSQL 16.0`. Further, for all the figures except Figure 9 under Section 7.3, we use no intra-query parallelism (single-threaded configuration of DuckDB and the no-parallel-worker configuration of PostgreSQL)


# Correctness

We follow the protocol below for correctness in the experimental evaluation section:

To verify correctness, after capturing provenance, we run the query only on the provenance, i.e., by replacing each input table access with its captured provenance. We then verify that the query result over the provenance is identical to the original query result over the database. Each input table access is considered unique in TraceProv and gets a separate backtrace query. Accordingly, for each input table access we only consider the provenance returned by the corresponding, unique backtrace query. As another check, we verify that GProM and TraceProv return the same rows as the provenance, if GProM's results are available.

Note: To check whether results over the provenance and over the database are identical, we dump the query result into a text file and apply the standard POSIX diff check.

### TPC-H:
We use the above methodology for both "P_all" and "P_single" tasks, on DuckDB and PostgreSQL. Specifically, for P_all, we verify that the captured provenance reproduces all the query result rows for all TPC-H queries (SF=1 and 10). For P_single, we verify that the captured provenance reproduces the query result row for which the provenance was requested, and we perform this check for all the query output rows for all TPC-H queries (SF=1 and 10). For both tasks, if GProM's results are available (for the corresponding SF and backend system, i.e., DuckDB or PostgreSQL), we verify that TraceProv and GProM return the same rows as the provenance.

# Analysis of Experimental Results

The general workflow of analysis of experimental results is to parse the produced JSON files produced during experimentation into DuckDB tables and generate plots. The respective code is available in `experiments/` directory.

### Provenance Polynomials:
We verify that each system generates an equivalent provenance polynomial for a given query.
