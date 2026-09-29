"""Device-status telemetry table tests without creating a Tk window."""
from pathlib import Path
from types import SimpleNamespace
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from underwater_console.app import MOTOR_STATUS_REFRESH_S, UnderwaterApp


class FakeTable:
    def __init__(self):
        self.rows = {
            f"motor-{index}": {"values": (f"motor {index}", "—", "—", "—", "等待反馈"), "tags": ()}
            for index in range(10)
        }

    def item(self, item, option=None, **kwargs):
        if option is not None:
            return self.rows[item][option]
        self.rows[item].update(kwargs)


class MotorStatusTableTests(unittest.TestCase):
    def setUp(self):
        self.app = UnderwaterApp.__new__(UnderwaterApp)
        self.app.motor_table = FakeTable()
        self.app.last_motor_refresh = 0.0
        self.status = SimpleNamespace(
            q=[index + 0.1 for index in range(10)],
            dq=[index + 0.2 for index in range(10)],
            tau=[index + 0.3 for index in range(10)],
            fresh_mask=0x3FF,
        )

    def test_updates_all_axes_and_marks_stale_feedback(self):
        self.status.fresh_mask &= ~(1 << 6)
        self.app.update_motor_status(SimpleNamespace(status=self.status), now=1.0)

        self.assertEqual(self.app.motor_table.rows["motor-3"]["values"],
                         ("motor 3", "3.1000", "3.2000", "3.3000", "正常"))
        self.assertEqual(self.app.motor_table.rows["motor-6"]["values"],
                         ("motor 6", "6.1000", "6.2000", "6.3000", "反馈过期"))
        self.assertEqual(self.app.motor_table.rows["motor-6"]["tags"], ("stale",))

    def test_refresh_is_limited_to_thirty_hz(self):
        report = SimpleNamespace(status=self.status)
        self.app.update_motor_status(report, now=1.0)
        self.status.q[0] = 9.0
        self.app.update_motor_status(report, now=1.0 + MOTOR_STATUS_REFRESH_S / 2)
        self.assertEqual(self.app.motor_table.rows["motor-0"]["values"][1], "0.1000")

        self.app.update_motor_status(report, now=1.0 + MOTOR_STATUS_REFRESH_S + 1e-6)
        self.assertEqual(self.app.motor_table.rows["motor-0"]["values"][1], "9.0000")

    def test_missing_stream_report_is_ignored_while_connecting(self):
        self.app.update_motor_status(None, now=1.0)
        self.app.update_motor_status(SimpleNamespace(status=None), now=2.0)

        self.assertEqual(self.app.last_motor_refresh, 0.0)
        self.assertEqual(self.app.motor_table.rows["motor-0"]["values"],
                         ("motor 0", "—", "—", "—", "等待反馈"))


if __name__ == "__main__":
    unittest.main()
