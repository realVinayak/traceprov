from traceprovpy.tests.utils import TestDbSetup
from traceprovpy.tools.benchmark import (
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
import os
import math

from traceprovpy.tools.run_with_timeout import RunParams


class TestBenchmark(TestDbSetup):

    def test_simple_run(self):
        benchmark = GenericBenchmark("simple-benchmark")
        directories = [
            QueryDirectory(
                dir_name="dir_1",
                queries=[
                    Query(
                        query_name="test_01",
                        spec=QuerySpec(base="base.sql", key="base_key"),
                    ),
                    Query(
                        query_name="test_02",
                        spec=QuerySpec(base="base.sql", key="base_key"),
                    ),
                ],
            ),
            QueryDirectory(
                dir_name="dir_2",
                queries=[
                    Query(
                        query_name="test_01",
                        spec=QuerySpec(base="base.sql", key="base_key"),
                    ),
                    Query(
                        query_name="test_02",
                        spec=QuerySpec(base="base.sql", key="base_key"),
                    ),
                ],
            ),
        ]
        result = benchmark.run(
            TestBenchmark.pg_user,
            TestBenchmark.pg_password,
            TestBenchmark.test_db,
            f"{os.getcwd()}/tests/test_queries/TestBenchmark",
            directories,
            params=RunParams(repeat=4, throwaway=1),
        )
        print(result)
        self.assertIn("dir_1", result)
        self.assertIn("dir_2", result)
        dir_1 = result["dir_1"]
        self.assertIn("test_01", dir_1)
        self.assertIn("test_02", dir_1)
        dir_2 = result["dir_2"]
        self.assertIn("test_01", dir_2)
        self.assertIn("test_02", dir_2)

        dir_1_test_01 = result["dir_1"]["test_01"]
        dir_1_test_02 = result["dir_1"]["test_02"]
        dir_2_test_01 = result["dir_2"]["test_01"]
        dir_2_test_02 = result["dir_2"]["test_02"]

        self.assertIn("base_key", dir_1_test_01)
        self.assertIn("base_key", dir_1_test_02)
        self.assertIn("base_key", dir_2_test_01)
        self.assertIn("base_key", dir_2_test_02)

        self.assertEqual(len(dir_1_test_01), 1)
        self.assertEqual(len(dir_1_test_02), 1)
        self.assertEqual(len(dir_2_test_01), 1)
        self.assertEqual(len(dir_2_test_02), 1)

        base_dir_1_test_01 = dir_1_test_01["base_key"]
        base_dir_1_test_02 = dir_1_test_02["base_key"]
        base_dir_2_test_01 = dir_2_test_01["base_key"]
        base_dir_2_test_02 = dir_2_test_02["base_key"]

        self.assertIn("base", base_dir_1_test_01)
        self.assertIn("base", base_dir_1_test_02)
        self.assertIn("base", base_dir_2_test_01)
        self.assertIn("base", base_dir_2_test_02)

        self.assertEqual(len(base_dir_1_test_01["base"]), 4)
        self.assertEqual(len(base_dir_1_test_02["base"]), 4)
        self.assertEqual(len(base_dir_2_test_01["base"]), 4)
        self.assertEqual(len(base_dir_2_test_02["base"]), 4)

        assert_close = lambda measured_times, expected: self.assertTrue(
            all(
                math.isclose(measured, expected, rel_tol=0.05)
                for measured in measured_times
            )
        )
        assert_close(base_dir_1_test_01["base"], 2)
        assert_close(base_dir_1_test_02["base"], 4)

        assert_close(base_dir_2_test_01["base"], 3)
        assert_close(base_dir_2_test_02["base"], 5)
