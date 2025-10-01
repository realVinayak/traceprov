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
    def assert_close(self, measured_times, expected):
        self.assertTrue(
            all(
                math.isclose(measured, expected, rel_tol=0.05)
                for measured in measured_times
            )
        )

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
        self.assertEqual(len(result), 2)
        dir_1 = result["dir_1"]
        self.assertIn("test_01", dir_1)
        self.assertIn("test_02", dir_1)
        self.assertEqual(len(dir_1), 2)
        dir_2 = result["dir_2"]
        self.assertIn("test_01", dir_2)
        self.assertIn("test_02", dir_2)
        self.assertEqual(len(dir_2), 2)

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

        self.assert_close(base_dir_1_test_01["base"], 2)
        self.assert_close(base_dir_1_test_02["base"], 4)

        self.assert_close(base_dir_2_test_01["base"], 3)
        self.assert_close(base_dir_2_test_02["base"], 5)

    def test_multiple_implementations_materialize(self):
        benchmark = GenericBenchmark("materialize-benchmark")
        directories = [
            QueryDirectory(
                dir_name="dir_1",
                queries=[
                    Query(
                        query_name="test_01",
                        spec=QuerySpec(base="base.sql", key="base_key"),
                    ),
                    Query(
                        query_name="test_01",
                        spec=QuerySpec(
                            base="impl1.sql",
                            key="impl1_key",
                            materialize="impl1.pseudo_material.sql",
                        ),
                    ),
                    Query(
                        query_name="test_02",
                        spec=QuerySpec(base="base.sql", key="base_key"),
                    ),
                    Query(
                        query_name="test_02",
                        spec=QuerySpec(
                            base="impl1.sql",
                            key="impl1_key",
                            materialize="impl1.pseudo_material.sql",
                        ),
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
                        query_name="test_01",
                        spec=QuerySpec(
                            base="impl1.sql",
                            key="impl1_key",
                            materialize="impl1.pseudo_material.sql",
                        ),
                    ),
                    Query(
                        query_name="test_02",
                        spec=QuerySpec(base="base.sql", key="base_key"),
                    ),
                    Query(
                        query_name="test_02",
                        spec=QuerySpec(
                            base="impl1.sql",
                            key="impl1_key",
                            materialize="impl1.pseudo_material.sql",
                        ),
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
        self.assertEqual(len(result), 2)
        dir_1 = result["dir_1"]
        self.assertIn("test_01", dir_1)
        self.assertIn("test_02", dir_1)
        self.assertEqual(len(dir_1), 2)
        dir_2 = result["dir_2"]
        self.assertIn("test_01", dir_2)
        self.assertIn("test_02", dir_2)
        self.assertEqual(len(dir_2), 2)

        dir_1_test_01 = result["dir_1"]["test_01"]
        dir_1_test_02 = result["dir_1"]["test_02"]
        dir_2_test_01 = result["dir_2"]["test_01"]
        dir_2_test_02 = result["dir_2"]["test_02"]

        self.assertIn("base_key", dir_1_test_01)
        self.assertIn("base_key", dir_1_test_02)
        self.assertIn("base_key", dir_2_test_01)
        self.assertIn("base_key", dir_2_test_02)

        self.assertIn("impl1_key", dir_1_test_01)
        self.assertIn("impl1_key", dir_1_test_02)
        self.assertIn("impl1_key", dir_2_test_01)
        self.assertIn("impl1_key", dir_2_test_02)

        self.assertEqual(len(dir_1_test_01), 2)
        self.assertEqual(len(dir_1_test_02), 2)
        self.assertEqual(len(dir_2_test_01), 2)
        self.assertEqual(len(dir_2_test_02), 2)

        # Check the base key implementation
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

        self.assert_close(base_dir_1_test_01["base"], 2)
        self.assert_close(base_dir_1_test_02["base"], 4)

        self.assert_close(base_dir_2_test_01["base"], 3)
        self.assert_close(base_dir_2_test_02["base"], 5)

        self.assertIn("materialize", base_dir_1_test_01)
        self.assertIn("materialize", base_dir_1_test_02)
        self.assertIn("materialize", base_dir_2_test_01)
        self.assertIn("materialize", base_dir_2_test_02)

        self.assertEqual(len(base_dir_1_test_01["materialize"]), 0)
        self.assertEqual(len(base_dir_1_test_02["materialize"]), 0)
        self.assertEqual(len(base_dir_2_test_01["materialize"]), 0)
        self.assertEqual(len(base_dir_2_test_02["materialize"]), 0)

        # Check the impl1 key implementation.
        impl1_dir_1_test_01 = dir_1_test_01["impl1_key"]
        impl1_dir_1_test_02 = dir_1_test_02["impl1_key"]
        impl1_dir_2_test_01 = dir_2_test_01["impl1_key"]
        impl1_dir_2_test_02 = dir_2_test_02["impl1_key"]

        self.assertIn("base", impl1_dir_1_test_01)
        self.assertIn("base", impl1_dir_1_test_02)
        self.assertIn("base", impl1_dir_2_test_01)
        self.assertIn("base", impl1_dir_2_test_02)

        self.assertEqual(len(impl1_dir_1_test_01["base"]), 4)
        self.assertEqual(len(impl1_dir_1_test_02["base"]), 4)
        self.assertEqual(len(impl1_dir_2_test_01["base"]), 4)
        self.assertEqual(len(impl1_dir_2_test_02["base"]), 4)

        self.assert_close(impl1_dir_1_test_01["base"], 2.5)
        self.assert_close(impl1_dir_1_test_02["base"], 4.5)

        self.assert_close(impl1_dir_2_test_01["base"], 3.5)
        self.assert_close(impl1_dir_2_test_02["base"], 5.5)

        self.assertIn("materialize", impl1_dir_1_test_01)
        self.assertIn("materialize", impl1_dir_1_test_02)
        self.assertIn("materialize", impl1_dir_2_test_01)
        self.assertIn("materialize", impl1_dir_2_test_02)

        self.assertEqual(len(impl1_dir_1_test_01["materialize"]), 4)
        self.assertEqual(len(impl1_dir_1_test_02["materialize"]), 4)
        self.assertEqual(len(impl1_dir_2_test_01["materialize"]), 4)
        self.assertEqual(len(impl1_dir_2_test_02["materialize"]), 4)

        self.assert_close(impl1_dir_1_test_01["materialize"], 2.7)
        self.assert_close(impl1_dir_1_test_02["materialize"], 4.7)

        self.assert_close(impl1_dir_2_test_01["materialize"], 3.7)
        self.assert_close(impl1_dir_2_test_02["materialize"], 5.7)
