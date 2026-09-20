from pathlib import Path

from traceprovpy.tools.extract_query_results import handle_duckdb_result
from traceprovpy.tools.file_utils import json_read_file
from traceprovpy.tools.normalized_row import ResultExtractor


def run():
    result_extractor = ResultExtractor()
    parsed = result_extractor.parse_args()

    out_dir = Path(parsed.out_dir)
    dirs, file_dirs = result_extractor.extract_notes()
    if parsed.dry_run:
        return
    assert parsed.dir is not None
    main_dir = Path(parsed.dir)

    version_counter = 0
    all_rows = []
    for current_dir, current_dir_files in zip(dirs, file_dirs, strict=True):
        new_version = current_dir[-1]
        for current_file in current_dir_files:
            version_counter += 1
            current_rows = []
            current_result = json_read_file(main_dir / current_file / "result.json")
            handle_duckdb_result(
                "traceprov",
                "offset",
                current_result,
                None,
                current_rows,
                version_counter,
            )
            for row in current_rows:
                assert hasattr(row, "category")
                if row.category != "traceprov":
                    print("Got category: ", row.category)
                    continue
                row.category = f"traceprov_{new_version}"
            all_rows.extend(current_rows)


if __name__ == "__main__":
    run()
