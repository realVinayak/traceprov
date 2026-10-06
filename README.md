# TraceProv

Derives the provenance of the query using user-defined functions and aggregates, and outperforms SOTA database intrusive systems.

## Correctness

We follow the protocol below for correctness in the experimental evaluation section:

To verify correctness, after capturing provenance, we run the query only on the provenance, i.e., by replacing each input table access with its captured provenance. We then verify that the query result over the provenance is identical to the original query result over the database. Each input table access is considered unique in TraceProv and gets a separate backtrace query. Accordingly, for each input table access we only consider the provenance returned by the corresponding, unique backtrace query. As another check, we verify that GProM and TraceProv return the same rows as the provenance, if GProM's results are available.

Note: To check whether results over the provenance and over the database are identical, we dump the query result into a text file and apply the standard POSIX diff check. 

### TPC-H:
We use the above methodology for both "P_all" and "P_single" tasks, on DuckDB and PostgreSQL. Specifically, for P_all, we verify that the captured provenance reproduces all the query result rows for all TPC-H queries (SF=1 and 10). For P_single, we verify that the captured provenance reproduces the query result row for which the provenance was requested, and we perform this check for all the query output rows for all TPC-H queries (SF=1 and 10). For both tasks, if GProM's results are available (for the corresponding SF and backend system, i.e., DuckDB or PostgreSQL), we verify that TraceProv and GProM return the same rows as the provenance.
