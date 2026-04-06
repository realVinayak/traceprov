from pathlib import Path

from traceprovpy.tools.run_duckdb_generic import just_read


def main():
    queries = map(str, range(1, 23))
    for query in queries:
        scale_1_query = just_read(Path(f"./scale_1/params_default/{query}/base.sql"))
        scale_10_query = just_read(Path(f"./scale_10/params_default/{query}/base.sql"))
        if scale_1_query != scale_10_query:
            print("diff at: ", query)


if __name__ == "__main__":
    main()
