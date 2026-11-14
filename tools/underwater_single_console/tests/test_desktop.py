"""A failed startup or Tk callback must fail unattended EXE verification."""
import json
import logging
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import desktop
import console


class DesktopTests(unittest.TestCase):
    def setUp(self):
        self.m_temp = tempfile.TemporaryDirectory()
        self.m_directory = Path(self.m_temp.name)
        self.m_environment = patch.dict(os.environ, {"LOCALAPPDATA": str(self.m_directory)})
        self.m_environment.start()
        self.m_report = self.m_directory / "report.json"

    def tearDown(self):
        for s_handler in logging.getLogger().handlers[:]:
            s_handler.close()
            logging.getLogger().removeHandler(s_handler)
        self.m_environment.stop()
        self.m_temp.cleanup()

    def test_dependency_failure_produces_nonzero_exit_and_report(self):
        with patch.object(desktop, "load_sdk", side_effect=ImportError("missing native SDK")):
            s_code = desktop.main(["--self-test", "--report", str(self.m_report)])
        self.assertEqual(s_code, 1)
        s_result = json.loads(self.m_report.read_text(encoding="utf-8"))
        self.assertFalse(s_result["ok"])
        self.assertIn("missing native SDK", s_result["error"])
        self.assertIn("missing native SDK", Path(s_result["log"]).read_text(encoding="utf-8"))

    def test_tk_callback_failure_is_not_reported_as_a_successful_smoke(self):
        def s_console(s_root, s_sdk):
            def s_fail():
                raise RuntimeError("callback failed")
            s_root.after(0, s_fail)
            s_root.after(20, s_root.destroy)
            # main schedules a later close as well; it should never fire here.
            return type("TestConsole", (), {"close": s_root.destroy})()

        with patch.object(desktop, "load_sdk", return_value=object()), patch.object(console, "Console", s_console):
            s_code = desktop.main(["--smoke", "--report", str(self.m_report)])
        self.assertEqual(s_code, 1)
        s_result = json.loads(self.m_report.read_text(encoding="utf-8"))
        self.assertFalse(s_result["ok"])
        self.assertEqual(s_result["callback_errors"], ["callback failed"])

    def test_version_query_does_not_load_sdk_or_create_window(self):
        with patch.object(desktop, "load_sdk") as s_load:
            s_code = desktop.main(["--version", "--report", str(self.m_report)])
        self.assertEqual(s_code, 0)
        s_load.assert_not_called()
        self.assertTrue(json.loads(self.m_report.read_text(encoding="utf-8"))["ok"])


if __name__ == "__main__":
    unittest.main()
