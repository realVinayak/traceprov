from unittest import TestCase

from traceprovpy.tools.run_with_timeout import ConnectionParams


class TestConnectionParams(TestCase):

    def test_flatten(self):
        connection_params = ConnectionParams(
            host="127.0.0.3",
            port="3243",
            user="postgresUser",
            password="postgresPassword",
            database="RandomDatabase",
        )

        self.assertEqual(
            connection_params.get_flat(),
            "-U postgresUser -h 127.0.0.3 -p 3243 -d RandomDatabase",
        )
