# TraceProv

Derives the provenance of the query using user-defined functions and aggregates, and outperforms SOTA database intrusive systems.

## Correctness

We follow the protocol below for correctness:

To verify correctness, after capturing provenance, we run the query just on the provenance, i.e., a subset of each input table of the query. We verify that the query result is the same. As another check, we verify that GProM and TraceProv return same rows as the provenance, if GProM's results are available.

The above methodology is used for both "P_all" and "P_single", i.e., deriving provenance of all rows or a single row.
