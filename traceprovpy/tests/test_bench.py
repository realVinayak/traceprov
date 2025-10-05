from traceprovpy.tests.utils import TestDbSetup
from traceprovpy.tools.benchmark import (
    ExtraQuery,
    GenericBenchmark,
    Query,
    QueryDirectory,
    QuerySpec,
)
import os
import math
import time
from traceprovpy.tools.run_with_timeout import RunParams


class TestBenchmark(TestDbSetup):
    def assert_close(self, measured_times, expected):
        # Assert that half of the measured times are close.
        self.assertGreaterEqual(
            len(
                [
                    1
                    for measured in measured_times
                    if math.isclose(measured, expected, rel_tol=0.05)
                ]
            ),
            len(measured_times) / 2,
        )

    def _simple_run(self, params: RunParams, do_length_check=True):
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
            params=params,
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

        if do_length_check:
            # In case where we're running it for a set amount of time,
            # we don't know beforehand what the length will be.
            self.assertEqual(len(base_dir_1_test_01["base"]), 4)
            self.assertEqual(len(base_dir_1_test_02["base"]), 4)
            self.assertEqual(len(base_dir_2_test_01["base"]), 4)
            self.assertEqual(len(base_dir_2_test_02["base"]), 4)

        self.assert_close(base_dir_1_test_01["base"], 2)
        self.assert_close(base_dir_1_test_02["base"], 4)

        self.assert_close(base_dir_2_test_01["base"], 3)
        self.assert_close(base_dir_2_test_02["base"], 5)

    def test_simple_run(self):
        params = RunParams(repeat=4, throwaway=1)
        self._simple_run(params)

    def test_simple_run_timed(self):
        start_time = time.perf_counter()
        params = RunParams(repeat=None, throwaway=None, execution_time=10)
        self._simple_run(params, do_length_check=False)
        end_time = time.perf_counter()
        print(end_time - start_time)

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
                        query_name="test_01",
                        spec=QuerySpec(
                            base="impl2.sql",
                            key="impl2_key",
                            materialize="impl2.pseudo_material.sql",
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
                    Query(
                        query_name="test_02",
                        spec=QuerySpec(
                            base="impl2.sql",
                            key="impl2_key",
                            materialize="impl2.pseudo_material.sql",
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
                        query_name="test_01",
                        spec=QuerySpec(
                            base="impl2.sql",
                            key="impl2_key",
                            materialize="impl2.pseudo_material.sql",
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
                    Query(
                        query_name="test_02",
                        spec=QuerySpec(
                            base="impl2.sql",
                            key="impl2_key",
                            materialize="impl2.pseudo_material.sql",
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

        self.assertIn("impl2_key", dir_1_test_01)
        self.assertIn("impl2_key", dir_1_test_02)
        self.assertIn("impl2_key", dir_2_test_01)
        self.assertIn("impl2_key", dir_2_test_02)

        self.assertEqual(len(dir_1_test_01), 3)
        self.assertEqual(len(dir_1_test_02), 3)
        self.assertEqual(len(dir_2_test_01), 3)
        self.assertEqual(len(dir_2_test_02), 3)

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

        # Check the impl2 key implementation.
        impl2_dir_1_test_01 = dir_1_test_01["impl2_key"]
        impl2_dir_1_test_02 = dir_1_test_02["impl2_key"]
        impl2_dir_2_test_01 = dir_2_test_01["impl2_key"]
        impl2_dir_2_test_02 = dir_2_test_02["impl2_key"]

        self.assertIn("base", impl2_dir_1_test_01)
        self.assertIn("base", impl2_dir_1_test_02)
        self.assertIn("base", impl2_dir_2_test_01)
        self.assertIn("base", impl2_dir_2_test_02)

        self.assertEqual(len(impl2_dir_1_test_01["base"]), 4)
        self.assertEqual(len(impl2_dir_1_test_02["base"]), 4)
        self.assertEqual(len(impl2_dir_2_test_01["base"]), 4)
        self.assertEqual(len(impl2_dir_2_test_02["base"]), 4)

        self.assert_close(impl2_dir_1_test_01["base"], 2.25)
        self.assert_close(impl2_dir_1_test_02["base"], 4.25)

        self.assert_close(impl2_dir_2_test_01["base"], 3.25)
        self.assert_close(impl2_dir_2_test_02["base"], 5.25)

        self.assertIn("materialize", impl2_dir_1_test_01)
        self.assertIn("materialize", impl2_dir_1_test_02)
        self.assertIn("materialize", impl2_dir_2_test_01)
        self.assertIn("materialize", impl2_dir_2_test_02)

        self.assertEqual(len(impl2_dir_1_test_01["materialize"]), 4)
        self.assertEqual(len(impl2_dir_1_test_02["materialize"]), 4)
        self.assertEqual(len(impl2_dir_2_test_01["materialize"]), 4)
        self.assertEqual(len(impl2_dir_2_test_02["materialize"]), 4)

        self.assert_close(impl2_dir_1_test_01["materialize"], 2.1)
        self.assert_close(impl2_dir_1_test_02["materialize"], 4.1)

        self.assert_close(impl2_dir_2_test_01["materialize"], 3.1)
        self.assert_close(impl2_dir_2_test_02["materialize"], 5.1)

    def test_extras(self):
        benchmark = GenericBenchmark("benchmarks-with-extras")
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
                            materialize="impl1_materialize.sql",
                            extras=[
                                ExtraQuery(
                                    label="impl1_count",
                                    query="$ROOT/tests/test_queries/TestBenchmarkWithExtras/test_01/impl1_materialize_count.sql",
                                    should_run=(
                                        lambda _, b_or_m: b_or_m == "materialize"
                                    ),
                                    skip_validation=True,
                                ),
                                ExtraQuery(
                                    label="impl1_fetch",
                                    query="$ROOT/tests/test_queries/TestBenchmarkWithExtras/test_01/impl1_fetch.sql",
                                    should_run=(
                                        lambda _, b_or_m: b_or_m == "materialize"
                                    ),
                                    skip_validation=True,
                                ),
                            ],
                        ),
                    ),
                ],
            )
        ]

        result = benchmark.run(
            TestBenchmark.pg_user,
            TestBenchmark.pg_password,
            TestBenchmark.test_db,
            f"{os.getcwd()}/tests/test_queries/TestBenchmarkWithExtras",
            directories,
            params=RunParams(repeat=4, throwaway=1),
        )

        print(result)

        self.assertEqual(len(result), 1)
        self.assertIn("dir_1", result)
        dir_1_result = result["dir_1"]

        self.assertEqual(len(dir_1_result), 1)
        self.assertIn("test_01", dir_1_result)

        dir_1_test_01_result = dir_1_result["test_01"]
        self.assertIn("base_key", dir_1_test_01_result)
        self.assertIn("impl1_key", dir_1_test_01_result)
        self.assertEqual(len(dir_1_test_01_result), 2)

        base_dir_1_test_01_result = dir_1_test_01_result["base_key"]
        self.assertEqual(len(base_dir_1_test_01_result["base"]), 4)
        self.assert_close(base_dir_1_test_01_result["base"], 3)
        self.assertEqual(len(base_dir_1_test_01_result["extras"]), 0)

        impl1_dir_1_test_01_result = dir_1_test_01_result["impl1_key"]
        self.assertEqual(len(impl1_dir_1_test_01_result["base"]), 4)
        self.assert_close(impl1_dir_1_test_01_result["base"], 2)
        self.assertEqual(len(impl1_dir_1_test_01_result["extras"]), 4)

        for extra in impl1_dir_1_test_01_result["extras"]:
            self.assertEqual(extra["impl1_count"]["captured"], [(5,)])
            self.assertEqual(
                extra["impl1_fetch"]["captured"], [(1,), (2,), (3,), (4,), (5,)]
            )
