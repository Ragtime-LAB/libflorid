"""Exercise the actual Tk controls with a mock SDK; never enumerate USB."""
from pathlib import Path
from types import SimpleNamespace
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from test_controller import FakeSdk, Mode, ControlKind
from underwater_console.arm_control import ArmControlPage, MODE_REQUEST_TEXT, joint_limits
from underwater_console.arm_stream import StreamReport
import tkinter as tk


class ArmPageTests(unittest.TestCase):
    def setUp(self):
        try:
            self.root = tk.Tk()
        except tk.TclError as exc:
            self.skipTest(f"Tk display not available: {exc}")
        self.root.withdraw()
        self.sent = []
        self.modes = []
        self.page = ArmControlPage(self.root, SimpleNamespace(sdk=FakeSdk(),
            publish_arm_command=self.sent.append), self.modes.append, lambda _: None)

    def tearDown(self):
        if hasattr(self, "root"):
            self.root.destroy()

    def status(self):
        return SimpleNamespace(q=[i / 10 for i in range(10)], fresh_mask=0x3FF,
            applied_mask=0, mode=Mode.Damp, command_reject_reason=0, control_stop_reason=0)

    def test_rows_defaults_motor_order_limits_and_gravity_toggle(self):
        self.assertEqual(len(self.page.rows), 10)
        self.page.update_stream(StreamReport(self.status()))
        self.assertEqual(self.sent[-1].q, self.status().q)
        self.assertEqual(self.sent[-1].kp, [20] * 10)
        self.assertEqual(self.sent[-1].kd, [0.5] * 10)
        self.assertEqual(self.sent[-1].tau, [0] * 10)
        self.assertEqual(self.sent[-1].dq, [0] * 10)
        self.assertEqual(self.sent[-1].kind, ControlKind.MitWithGravity)
        self.page.gravity.set(False)
        self.page.publish()
        self.assertEqual(self.sent[-1].kind, ControlKind.MIT)
        for i, limits in enumerate(joint_limits()):
            self.assertEqual(float(self.page.rows[i]["slider"]["from"]), limits["lower"])
            self.assertEqual(float(self.page.rows[i]["slider"]["to"]), limits["upper"])

    def test_slider_and_entry_change_only_the_selected_motor(self):
        self.page.update_stream(StreamReport(self.status()))
        self.page.slide(7, -1.25)
        self.assertEqual(self.sent[-1].q[7], -1.25)
        self.assertEqual(self.sent[-1].q[2], 0.2)
        for field, value in (("tau", -1.3), ("dq", -0.15), ("kp", 35), ("kd", 0.75)):
            self.page.rows[9]["values"][field].set(str(value))
            self.assertTrue(self.page.commit(9, field))
            self.assertEqual(getattr(self.sent[-1], field)[9], value)
        # New telemetry never overwrites the user's chosen target.
        self.page.update_stream(StreamReport(self.status(), True))
        self.assertEqual(self.sent[-1].q[7], -1.25)
        self.page.use_measured_angles()
        self.assertEqual(self.sent[-1].q, self.status().q)

    def test_invalid_numeric_input_is_visible_without_clipping_or_blocking_disable(self):
        row = self.page.rows[1]
        for value in ("abc", "nan", "1e100", "3"):
            row["values"]["q"].set(value)
            self.assertFalse(self.page.commit(1, "q"))
            self.assertIn("未应用", self.page.feedback.get())
            self.assertEqual(row["applied"]["q"], 0)
        self.page.switch_mode("disable")
        self.assertEqual(self.modes, ["disable"])
        self.page.switch_mode("enter_control")
        self.assertEqual(self.modes, ["disable"])

    def test_four_mode_buttons_route_rpcs_and_commit_pending_inputs(self):
        self.page.rows[4]["values"]["tau"].set("0.35")
        for mode, method in (("Damp", "disable"), ("Gravity", "enable"),
                             ("Teleop", "enter_teleop"), ("Control", "enter_control")):
            self.page.mode_buttons[mode].invoke()
            self.assertEqual(self.page.feedback.get(), MODE_REQUEST_TEXT[method])
        self.assertEqual(self.modes, ["disable", "enable", "enter_teleop", "enter_control"])
        self.assertEqual(self.sent[-1].tau[4], 0.35)

    def test_status_refresh_between_mouse_press_and_release_does_not_cancel_click(self):
        button = self.page.mode_buttons["Gravity"]
        button.event_generate("<Enter>", x=5, y=5)
        button.event_generate("<ButtonPress-1>", x=5, y=5)
        self.root.update()

        self.page.update_mode(SimpleNamespace(mode=Mode.Damp))
        self.root.update()
        button.event_generate("<ButtonRelease-1>", x=5, y=5)
        self.root.update()

        self.assertEqual(self.modes, ["enable"])
        self.assertIn("selected", self.page.mode_buttons["Damp"].state())
        self.assertNotIn("selected", button.state())

    def test_outside_limit_feedback_is_not_silently_clamped_to_command(self):
        status = self.status()
        status.q[1] = 2.4
        self.page.update_stream(StreamReport(status))
        self.assertEqual(self.sent[-1].q[1], 2.4)
        self.assertIn("超出滑条范围", self.page.feedback.get())


if __name__ == "__main__":
    unittest.main()
