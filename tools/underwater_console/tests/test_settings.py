"""Settings form parsing and confirmation tests, without opening a Tk window."""
from pathlib import Path
from types import SimpleNamespace
import sys
import struct
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
try:
    from underwater_console.settings import GROUPS, SettingsPage, parse_settings, format_float32
except ImportError:
    GROUPS = SettingsPage = parse_settings = None


class Variable:
    def __init__(self, value=""):
        self.value = value

    def get(self):
        return self.value

    def set(self, value):
        self.value = value


def settings_type():
    fields = {field: [0.0] * count for field, count in (
        ("angle_scale", 5), ("angle_offset_rad", 5), ("kp", 5), ("kd", 5),
        ("gravity_scale", 8), ("friction_coulomb_positive_nm", 10),
        ("friction_coulomb_negative_nm", 10), ("friction_viscous_nm_per_rad_s", 10),
        ("friction_deadband_rad_s", 10), ("friction_transition_rad_s", 10),
        ("friction_filter_time_constant_s", 10), ("force_gain", 5),
        ("force_low_gain", 5), ("force_velocity_filter_s", 5), ("force_load_filter_s", 5),
    )}
    return SimpleNamespace(**fields, force_low_load_nm=2.0, force_high_load_nm=5.0, buzzer_alarm_enabled=True)


@unittest.skipIf(SettingsPage is None, "optional tkinter package not installed")
class SettingsFormTests(unittest.TestCase):
    def setUp(self):
        self.actions = []
        self.sdk = SimpleNamespace(UnderwaterSettings=settings_type)
        cells = {}
        for group, rows in GROUPS:
            for label, field, start, count in rows:
                for i in range(count):
                    index = None if start is None else start + i
                    cells[field, index] = Variable(str(len(cells) + 0.25)), group + " / " + label
        self.page = SimpleNamespace(cells=cells, controller=SimpleNamespace(sdk=self.sdk,
            apply_settings=self.actions.append), log=lambda _: None,
            feedback=Variable(), source=Variable(), buzzer_alarm_enabled=Variable(True), winfo_toplevel=lambda: None)

    def test_all_110_cells_round_trip_including_signed_scales(self):
        self.assertEqual(len(self.page.cells), 110)
        self.page.cells["angle_scale", 2][0].set("-1.5")
        self.page.cells["gravity_scale", 5][0].set("-0.5")
        dto = parse_settings(self.sdk, self.page.cells)
        for (field, index), (variable, _) in self.page.cells.items():
            actual = getattr(dto, field)
            self.assertEqual(actual if index is None else actual[index], float(variable.get()))

    def test_muted_device_readback_populates_checkbox(self):
        values = settings_type()
        values.buzzer_alarm_enabled = False
        result = SimpleNamespace(error="None_", device_status=0, reason="",
            snapshot=SimpleNamespace(settings=values, revision=3, source="Stored", storage_error=0))
        SettingsPage.update_result(self.page, result, "settings")
        self.assertIs(self.page.buzzer_alarm_enabled.get(), False)

    def test_float_display_is_short_and_preserves_all_float32_bits(self):
        self.assertEqual(format_float32(struct.unpack("<f", struct.pack("<f", 0.03))[0]), "0.03")
        self.assertEqual(format_float32(1.5), "1.5")
        for bits in (1, 0x80000000, 0x7F7FFFFF, 0x00800000, 0x3EAAAAAB, 0xC0490FDB):
            encoded = struct.pack("<I", bits)
            value = struct.unpack("<f", encoded)[0]
            self.assertEqual(struct.pack("<f", float(format_float32(value))), encoded)

    def test_cancel_confirmation_sends_no_disable_or_settings(self):
        with patch("underwater_console.settings.messagebox.askokcancel", return_value=False) as confirm:
            SettingsPage.apply(self.page)
        self.assertEqual(confirm.call_count, 1)
        self.assertEqual(self.actions, [])
        self.assertIn("未发送", self.page.feedback.get())

    def test_confirmation_submits_exact_form_once(self):
        self.page.buzzer_alarm_enabled.set(False)
        with patch("underwater_console.settings.messagebox.askokcancel", return_value=True):
            SettingsPage.apply(self.page)
        self.assertEqual(len(self.actions), 1)
        self.assertIs(self.actions[0].buzzer_alarm_enabled, False)
        self.assertEqual(self.actions[0].gravity_scale[7],
                         float(self.page.cells["gravity_scale", 7][0].get()))

    def test_invalid_number_is_reported_before_confirmation_without_clipping(self):
        for value in ("NaN", "Infinity", "invalid", "1e100"):
            self.page.cells["kp", 2][0].set(value)
            with patch("underwater_console.settings.messagebox.askokcancel") as confirm:
                SettingsPage.apply(self.page)
            confirm.assert_not_called()
            self.assertIn("Kp", self.page.feedback.get())
        self.assertEqual(self.actions, [])

    def test_rejected_write_preserves_edits_and_shows_storage_metadata(self):
        before = self.page.cells["kp", 2][0].get()
        self.page.buzzer_alarm_enabled.set(False)

        class Reply:
            error = "DeviceRejected"
            device_status = 2
            reason = "nvs write failed"
            snapshot = SimpleNamespace(settings=settings_type(), revision=7, source="Stored", storage_error=-5)
            def __bool__(self):
                return False

        SettingsPage.update_result(self.page, Reply(), "set_settings")
        self.assertEqual(before, self.page.cells["kp", 2][0].get())
        self.assertIs(self.page.buzzer_alarm_enabled.get(), False)
        self.assertIn("nvs write failed", self.page.feedback.get())
        self.assertIn("storage_error=-5", self.page.source.get())

    def test_usable_settings_with_storage_errors_are_displayed_and_editable(self):
        for source in ("Corrupt", "StorageError"):
            values = settings_type()
            values.gravity_scale[5] = -0.5
            result = SimpleNamespace(error="None_", device_status=0, reason="using compiled settings",
                snapshot=SimpleNamespace(settings=values, revision=0, source=source, storage_error=-5))
            SettingsPage.update_result(self.page, result, "settings")
            self.assertEqual(self.page.cells["gravity_scale", 5][0].get(), "-0.5")
            self.assertIn(source, self.page.source.get())
            with patch("underwater_console.settings.messagebox.askokcancel", return_value=True):
                SettingsPage.apply(self.page)
        self.assertEqual(len(self.actions), 2)

    def test_default_response_only_fills_form_without_sending_disable_or_write(self):
        result = SimpleNamespace(error="None_", device_status=0, reason="compiled defaults",
            snapshot=SimpleNamespace(settings=settings_type(), revision=0, source="Defaults", storage_error=0))
        self.page.buzzer_alarm_enabled.set(False)
        SettingsPage.update_result(self.page, result, "default_settings")
        self.assertIs(self.page.buzzer_alarm_enabled.get(), True)
        self.assertIn("尚未写入", self.page.source.get())
        self.assertEqual(self.actions, [])


if __name__ == "__main__":
    unittest.main()
