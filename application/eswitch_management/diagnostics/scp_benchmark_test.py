import csv
import hashlib
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import scp_benchmark


class BenchmarkTests(unittest.TestCase):
    def invoke(self, output, expected, actual):
        argv = ["benchmark", "--remote", "ubuntu@test:/tmp/payload", "--label",
                "ct", "--bytes", str(len(expected)), "--sha256",
                hashlib.sha256(expected).hexdigest(), "--runs", "2",
                "--output", str(output)]
        downloads = []

        def copy(command, check):
            self.assertTrue(check)
            self.assertIn("Compression=no", command)
            self.assertIn("ControlMaster=no", command)
            self.assertNotIn("StrictHostKeyChecking=no", command)
            destination = Path(command[-1])
            downloads.append(destination)
            destination.write_bytes(actual)

        with patch("sys.argv", argv), patch("subprocess.run", side_effect=copy):
            scp_benchmark.main()
        return downloads

    def test_warmup_integrity_and_cleanup(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "results.csv"
            downloads = self.invoke(output, b"test-payload", b"test-payload")
            with output.open() as results:
                rows = list(csv.DictReader(results))
            self.assertEqual([row["warmup"] for row in rows], ["1", "0", "0"])
            self.assertTrue(all(row["sha256_ok"] == "1" for row in rows))
            self.assertTrue(all(not path.exists() for path in downloads))
            with self.assertRaises(FileExistsError):
                self.invoke(output, b"test-payload", b"test-payload")

    def test_reject_corrupt_download(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "results.csv"
            with self.assertRaises(RuntimeError):
                self.invoke(output, b"good", b"evil")
            with output.open() as results:
                rows = list(csv.DictReader(results))
            self.assertEqual(len(rows), 1)
            self.assertEqual(rows[0]["sha256_ok"], "0")


if __name__ == "__main__":
    unittest.main()
