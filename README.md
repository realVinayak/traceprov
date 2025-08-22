# TraceProv

Derives primary keys of the base table that affect the final query result.

The derivation is performed in two steps:

1. Forward tracing
    - Done during query execution, using user-defined functions

2. Backward inferring
    - Inferring the primary keys from the traces generated

# User-defined Functions

Below is the list of user-defined functions used in the forward tracing. The aggregates are marked with [*Aggregation*], and functions are marked with [*Function*]. All the aggregates, and functions, are parallel-safe and Postgres is able to utilize them as part of parallel (and partial) aggregation. The SQL definitions can be found in [parallel_drop_and_create.sql](https://github.com/realVinayak/traceprov/blob/main/postgres/parallel_drop_and_create.sql).

1. [*Aggregation*] `agg_map_parallel(BIGINT pk[])`
    - Aggregates over the input primary key(s) and returns a pointer that represents the group.
    - For joins, and cases where multiple keys form the primary key, more than 1 argument can be passed.

2. [*Aggregation*] `agg_map_parallel_second(BIGINT pk[])`
    - Same functionality as `agg_map_parallel`. In addition, it also records that a new `group by` is being captured.
    - This is used when a `group by` unrelated to a previously captured `group by` (via `agg_map_parallel`), is being captured.

3. [*Aggregation*] `agg_map_parallel_tag(BIGINT pk[])`
    - Same functionality as `agg_map_parallel`. In addition, the returned group pointers are tagged with the worker id. Specifically, the first byte in the memory address stores the worker id (computed outside of Postgres).

4. [*Aggregation*] `agg_from_ptr(BIGINT)`
    - Aggregates over the **pointer** returned from `agg_map_parallel` or `agg_map_parallel_second`, and returns a new pointer that represents the group.
    - This is used when there is a nested aggregation. The "inner" aggregation gets captured using `agg_map_parallel` (or `agg_map_parallel_second`), and the pointers returned get aggregated over ("outer" aggregation) with this function (`agg_from_ptr`).

5. [*Aggregation*] `agg_from_ptr_tag(BIGINT)`
    - Same functionality as `agg_from_ptr`. In addition, it performs de-tagging to compute correct memory address.
    - Used in conjunction with `agg_map_parallel_tag`.

6. [*Function*] `log_subquery_pk(BIGINT pk[])`
    - Simply logs the incoming primary key(s) in a separate file.
    - Used in `EXISTS` where the primary keys of records that satisfy the predicate need to be logged.
    - Returns `true`.

7. [*Function*] `log_subquery_pk_neg(BIGINT pk[])`
    - Same functionality as `log_subquery_pk`. In addition, it returns `false` instead of `true`.
    - Used in `NOT EXISTS`.
    - Returns `false`.

8. [*Function*] `mark_later(BIGINT)`
    - The core function that sets the input pointer as "present".
    - The input pointer can be from `agg_map_parallel`, `agg_map_parallel_second`, `agg_map_parallel_tag` or `agg_from_ptr` (all of the aggregates defined previously).

9. [*Function*] `reinit_state(INTEGER)`
    - During forward tracing, files get created for storing the traces.
    - This function should be called before a new query runs to remove the created files from a previous forward trace.

# Building the UDFs

The UDFs (and aggregates) are coded in [postgres/agg_maps.c](https://github.com/realVinayak/traceprov/blob/main/postgres/agg_maps.c).

To build the UDFs, do the following (with a suffix chosen for `SOME_SUFFIX`).

```
git pull https://github.com/realVinayak/traceprov.git
cd postgres
sudo make module src=agg_maps suff=<SOME_SUFFIX>
```

This will do the following actions:
1. Create the UDF shared-library executable (.so).
2. Copy the executable to `/usr/lib/postgresql/14/lib` for loading.
3. Create a SQL file (using `parallel_drop_and_create.sql` as template) to create the functions and aggregates.
4. Copy the created SQL file to `/home/postgres`.

Thus, to load the functions (and aggregates), in Postgres simply do the following:

```
\i /home/postgres/parallel_drop_and_create_agg_maps_<SOME_SUFFIX>.auto.sql
```

# Performing inference

The code for inference is in [postgres/infer.cpp](https://github.com/realVinayak/traceprov/blob/main/postgres/infer.cpp).

To perform inference, after forward tracing, first build the infer executable.

```
cd postgres
make infer
```

For debugging, use `make infer_dbg`.

`infer.o` executable will be created, and can be used in a standalone fashion, outside of Postgres.

## Options for infer

The executable `infer.o` can be run simply with 

```
./infer.o
```

Without any options, it will compute the primary keys that contribute to the final result, but it will **not** store them. 

Further, it will only look at the main trace file, and not the log_subquery_pk(neg) files. See options `-s.*` below to output subquery pks.

Following options are supported for the executable


1. `-f <filename.ids>`
    - `filename.ids: string`
    - Store the derived primary keys in `filename.ids`.

2. `-g <group_no>`
    - `group_no: int`
    - In cases where multiple unrelated groups are present, specify the group number. See TPC-H query 11.

3. `-s.num <length>`
    - `length: int`
    - In cases where `log_subquery_pk(neg)` is used, specify the number of arguments.
    - For example, if the call expression is `log_subquery_pk(o_orderkey, l_linenumber, l_orderkey)`, use `-s.num 3`.

4. `-s.out <filename.ids>`
    - `filename.ids: string`
    - Store the derived primary keys (from subquery) in `filename.ids`.

5. `-s.find <should_find>`
    - `should_find: 0 or 1`
    - Set this to `1` if the primary keys from the subquery need to be matched primary keys from the main trace file.
    - Hard-coded to match the last key in subquery to the first key in the main trace file.

6. `-ig <should_ignore_group_no>`
    - `should_ignore_group_no: 0 or 1`
    - In some cases, the group number in the trace files is not used (for example, in query 17 of TPC-H).
    - Set this to 1 to ignore the trace file's group number.

# TPC-H Benchmarks 

Each TPC-H query should be run with different parameters to get an accurate picture of the overhead, since the technique is sensitive to the number of PKs logged.

## Base queries
The base queries used for Postgres (without parameter substitution) can be found in [postgres/benchmark/tpch/templates/base](https://github.com/realVinayak/traceprov/tree/main/postgres/benchmark/tpch/templates/base).

## Forward trace queries
The queries used to compute the trace (without parameter substitution) can be found in [postgres/benchmark/tpch/templates/provtrace](https://github.com/realVinayak/traceprov/tree/main/postgres/benchmark/tpch/templates/provtrace). The queries are augmented with the user-defined functions and aggregates for tracing.

## Validation queries
To verify that the computed provenance (set of PKs) is sufficient to return the same result compared to the base query, validation queries are run just over the PKs. These queries, without parameter substitution, can be found in [postgres/benchmark/tpch/templates/validate](https://github.com/realVinayak/traceprov/tree/main/postgres/benchmark/tpch/templates/validate).

# Running TPC-H benchmarks

It is **completely** possible to run the TPC-H benchmarks using the `./infer.o` executable directly. However, this process is quite manual.

Thus, a test harness is developed to run the TPC-H benchmarks. This can be found in [postgres/benchmark/tpch/run_bench.py](https://github.com/realVinayak/traceprov/blob/main/postgres/benchmark/tpch/run_bench.py). The script can be run with

```
python3 <path-to-config.json> <path-to-validate-configs> <path-to-infer-executable>
```


## 1. `<path-to-config.json>`

This is the core config that defines which TPC-H queries run. For example, below is config for scale 1 TPC-H.

```
{
    "db_name": "tpch_01_v01",
    "queries": [
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
        12,
        13,
        14,
        17,
        19
    ],
    "validate": false,
    "repeat": 10,
    "subdirs": [
        "params_2",
        "params_3"
    ],
    "validate_steps": {
        "11": [
            "a",
            "b"
        ]
    },
    "expected_diff_failures": [
        "11"
    ]
}
```

### Key explanations

1. **db_name**
    - The database to use to run the queries against.
2. **queries**
    - List of queries to run.
3. **validate**
    - Whether it should validate the computed provenance or not, by running the query on the entire database, and just the computed provenance, and asserting that the output is same.
4. **repeat**
    - Number of times a query should be performed.
5. **subdirs**
    - List of subdirs it should look at to find a query (different parameters for the same query can be specificied this way)
    - In the example above, it will try to find each query in `params_2/` and `params_3` directory.
6. **validate_steps**:
    - Some queries need go through multiple phases for validation
    - Occurs when unnrelated groups are present, and inference needs to run multiple times. The total time of inference is computed.
    - In the example above, it indicates *"query 11 has two stages, named a and b"*.
7. **expected_diff_failures**:
    - In some cases, the text output of base query result and provenance-based query are different, but results are actually correct. 
    - Here, indicate which queries have such diff errors.

## 2. `<path-to-validate-configs>`
For validation, directives on how the IDs should be provided.

For example, consider below query 1 of TPC-H

```
SELECT
  l_returnflag,
  l_linestatus,
  sum(l_quantity) AS sum_qty,
  sum(l_extendedprice) AS sum_base_price,
  sum(l_extendedprice * (1 - l_discount)) AS sum_disc_price,
  sum(l_extendedprice * (1 - l_discount) * (1 + l_tax)) AS sum_charge,
  avg(l_quantity) AS avg_qty,
  avg(l_extendedprice) AS avg_price,
  avg(l_discount) AS avg_disc,
  count(*) AS count_order
FROM
  lineitem
WHERE
  l_shipdate <= date '1998-12-01' - INTERVAL '90' DAY
GROUP BY
  l_returnflag,
  l_linestatus
ORDER BY
  l_returnflag,
  l_linestatus;
```

We compute the primary keys of lineitem that yield the result. Thus, for validation, we'd need to run below query, where (%A%) gets substituted for the primary keys.

```
SELECT
  l_returnflag,
  l_linestatus,
  sum(l_quantity) AS sum_qty,
  sum(l_extendedprice) AS sum_base_price,
  sum(l_extendedprice * (1 - l_discount)) AS sum_disc_price,
  sum(l_extendedprice * (1 - l_discount) * (1 + l_tax)) AS sum_charge,
  avg(l_quantity) AS avg_qty,
  avg(l_extendedprice) AS avg_price,
  avg(l_discount) AS avg_disc,
  count(*) AS count_order
FROM
  lineitem
WHERE
  l_shipdate <= date '1998-12-01' - INTERVAL '90' DAY
  AND (l_orderkey, l_linenumber) in (%A%)
GROUP BY
  l_returnflag,
  l_linestatus
ORDER BY
  l_returnflag,
  l_linestatus;
```

The information about how it should be substituted is provided in `1.validate.config.sql`, like below:

```
{
    "pk_order": [
        "l_orderkey",
        "l_linenumber"
    ],
    "inserts": [
        {
            "ref": "A",
            "keys": [
                "l_orderkey",
                "l_linenumber"
            ]
        }
    ]
}
```

Here, `pk_order` stores how the primary keys are stored during forward tracing.
In `keys` of an `inserts` object, the substitution order is described, for a specific ref (in this case, A).